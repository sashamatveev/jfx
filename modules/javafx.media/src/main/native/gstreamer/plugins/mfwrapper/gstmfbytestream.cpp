/*
 * Copyright (c) 2024, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include "gstmfbytestream.h"

#define ENABLE_TRACE 1

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#if ENABLE_TRACE
    #define TRACE g_print
#else // ENABLE_TRACE
    #define TRACE
#endif // ENABLE_TRACE

CGSTMFByteStream::CGSTMFByteStream(HRESULT &hr, QWORD qwLength, GstPad *pSinkPad)
{
    hr = S_OK;

    // If length is not provided we assume it is fMP4
    if (qwLength == -1)
        m_bfMP4 = TRUE;
    else
        m_bfMP4 = FALSE;

    m_ulRefCount = 0;
    m_qwPosition = 0;
    //m_qwLength = m_bfMP4 ? 0 : qwLength;
    m_qwLength = qwLength;
    m_qwSegmentPosition = 0;
    m_qwSegmentLength = -1;

    m_pBytes = NULL;
    m_cbBytes = 0;
    m_cbBytesRead = 0;
    m_pCallback = NULL;
    m_pAsyncResult = NULL;
    m_readResult = S_OK;
    m_bWaitForEvent = FALSE;
    m_bIsEOS = FALSE;
    m_bIsEOSEventReceived = FALSE;
    m_bIsReload = FALSE;

    m_pSinkPad = pSinkPad;

    InitializeCriticalSection(&m_csLock);

    // IMFMediaEventGenerator
    InitializeCriticalSection(&m_csEventLock);
    hr = MFCreateEventQueue(&m_pEventQueue);
    m_bEventQueueShutdown = FALSE;
}

CGSTMFByteStream::~CGSTMFByteStream()
{
    DeleteCriticalSection(&m_csLock);
}

HRESULT CGSTMFByteStream::ReadRangeAvailable()
{
    Lock();
    BOOL bWaitForEvent = m_bWaitForEvent;
    m_bWaitForEvent = FALSE;
    Unlock();

    if (bWaitForEvent)
        return ReadData();
    else
        return S_FALSE;
}

void CGSTMFByteStream::SetSegmentLength(QWORD qwSegmentLength, bool bForce)
{
    Lock();
    if (bForce || m_bWaitForEvent)
    {
        m_qwSegmentLength = qwSegmentLength;
        m_qwSegmentPosition = 0;
        // if (qwSegmentLength != -1)
        //     m_qwLength += qwSegmentLength;
    }
    Unlock();
}

// Even if we reporting MFBYTESTREAM_IS_SEEKABLE to MF to make it happy (will not
// initialized otherwise), we can only issue seek on MF source reader if length
// is known (HTTP/FILE). For HLS we wil forward seek event upstream to handle
// seek.
bool CGSTMFByteStream::IsSeekSupported()
{
    return !m_bfMP4;
}

HRESULT CGSTMFByteStream::CompleteReadData(HRESULT hr)
{
    g_print("AMDEBUG CGSTMFByteStream::CompleteReadData() 0x%X m_pCallback %p m_pAsyncResult %p\n", hr, m_pCallback, m_pAsyncResult);
    m_readResult = hr;
    if (m_pCallback && m_pAsyncResult)
        return m_pCallback->Invoke(m_pAsyncResult);

    return S_OK;
}

void CGSTMFByteStream::SignalEOS()
{
    m_bIsEOSEventReceived = TRUE;
}

void CGSTMFByteStream::ClearEOS()
{
    m_bIsEOS = FALSE;
    m_bIsEOSEventReceived = FALSE;
}

BOOL CGSTMFByteStream::IsReload()
{
    return m_bIsReload;
}

// IMFByteStream
HRESULT CGSTMFByteStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    HRESULT hr = S_OK;

    if (pb == NULL || pCallback == NULL)
        return E_POINTER;

    if (m_pSinkPad == NULL)
        return E_POINTER;

    if (m_readResult != S_OK)
        return m_readResult; // Do not start new read if old one failed.

    TRACE("JFXMEDIA CGSTMFByteStream::BeginRead() cb: %lu m_qwSegmentLength: %llu m_qwSegmentPosition: %llu m_qwPosition: %llu m_qwLength: %llu\n", cb, m_qwSegmentLength, m_qwSegmentPosition, m_qwPosition, m_qwLength);

    // Save read request
    m_pBytes = pb;
    m_cbBytes = cb;
    m_cbBytesRead = 0;
    m_pCallback = pCallback;

    // Create async result object to signal read completion
    hr = MFCreateAsyncResult(NULL, pCallback, punkState, &m_pAsyncResult);
    if (FAILED(hr))
        return hr;

    return ReadData();
}

HRESULT CGSTMFByteStream::BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    return E_NOTIMPL;
}

HRESULT CGSTMFByteStream::Close()
{
    // Nothing to close
    return S_OK;
}

HRESULT CGSTMFByteStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead)
{
    Lock();
    m_bWaitForEvent = FALSE;
    Unlock();

    if (pResult != NULL)
        pResult->SetStatus(m_readResult);

    *pcbRead = m_cbBytesRead;

    m_pCallback = NULL;
    m_pAsyncResult = NULL;

    TRACE("JFXMEDIA CGSTMFByteStream::EndRead() m_cbBytesRead: %lu\n", m_cbBytesRead);

    return S_OK;
}

HRESULT CGSTMFByteStream::EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten)
{
    return E_NOTIMPL;
}

HRESULT CGSTMFByteStream::Flush()
{
    return E_NOTIMPL;
}

HRESULT CGSTMFByteStream::GetCapabilities(DWORD *pdwCapabilities)
{
    (*pdwCapabilities) |= MFBYTESTREAM_IS_READABLE;
    (*pdwCapabilities) |= MFBYTESTREAM_IS_SEEKABLE;
    (*pdwCapabilities) |= MFBYTESTREAM_IS_REMOTE;
    return S_OK;
}

HRESULT CGSTMFByteStream::GetCurrentPosition(QWORD *pqwPosition)
{
//    if (m_qwLength != -1)
        (*pqwPosition) = m_qwPosition;
    // else if (m_bIsEOS)
    //     (*pqwPosition) = m_qwSegmentPosition;
    // else
    //     (*pqwPosition) = -1;
    g_print("AMDEBUG CGSTMFByteStream::GetCurrentPosition() %llu\n", (*pqwPosition));
    return S_OK;
}

HRESULT CGSTMFByteStream::GetLength(QWORD *pqwLength)
{
    if (m_bfMP4 && !m_bIsEOS)
    {
//        if (m_qwLength == 0)
            (*pqwLength) = -1;
        // else
        //     (*pqwLength) = m_qwLength + 1 + 262144;
    }
    else
        (*pqwLength) = m_qwLength;
    // if (!m_bfMP4)
    //     (*pqwLength) = m_qwLength;
    // // else if (m_bIsEOS)
    // //     (*pqwLength) = m_qwSegmentLength;
    // else
    //     (*pqwLength) = -1;
    g_print("AMDEBUG CGSTMFByteStream::GetLength() %llu\n", (*pqwLength));
    return S_OK;
}

HRESULT CGSTMFByteStream::IsEndOfStream(BOOL *pfEndOfStream)
{
    if (m_bIsEOS)
        (*pfEndOfStream) = TRUE;
    else if (m_qwPosition >= m_qwLength)
        (*pfEndOfStream) = TRUE;
    else
        (*pfEndOfStream) = FALSE;

    g_print("AMDEBUG CGSTMFByteStream::IsEndOfStream() %d\n", (*pfEndOfStream));

    return S_OK;
}

HRESULT CGSTMFByteStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
    return E_NOTIMPL;
}

HRESULT CGSTMFByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition)
{
    HRESULT hr = S_OK;

    if (pqwCurrentPosition == NULL)
        return E_POINTER;

    QWORD qwSeekPosition = 0;
    switch (SeekOrigin)
    {
        case msoBegin:
            qwSeekPosition = llSeekOffset;
            break;
        case msoCurrent:
            qwSeekPosition = m_qwPosition + llSeekOffset;
            break;
        default:
            return E_FAIL;
    }

    hr = SetCurrentPosition(qwSeekPosition);
    if (FAILED(hr))
        return hr;

    *pqwCurrentPosition = m_qwPosition;

    return S_OK;
}

HRESULT CGSTMFByteStream::SetCurrentPosition(QWORD qwPosition)
{
    g_print("AMDEBUG CGSTMFByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu\n", qwPosition, m_qwPosition);
    if (qwPosition > m_qwLength)
    {
        g_print("AMDEBUG CGSTMFByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu E_INVALIDARG\n", qwPosition, m_qwPosition);
        return E_INVALIDARG;
    }

    // if (m_bfMP4 && m_bIsEOS && qwPosition > m_qwPosition)
    // {
    //     g_print("AMDEBUG CGSTMFByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu E_INVALIDARG\n", qwPosition, m_qwPosition);
    //     return E_INVALIDARG;
    // }

    if (m_qwPosition == qwPosition)
    {
        g_print("AMDEBUG CGSTMFByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu S_OK\n", qwPosition, m_qwPosition);
        return S_OK;
    }

    if (!m_bfMP4)
    {
        m_qwPosition = qwPosition;
    }
    else if (qwPosition == 0)
    {
        // During initialization MF will re-read from 0 several times, so
        // if position request for 0 reset segment position as well.
        m_qwPosition = 0;
        m_qwSegmentPosition = 0;
    }
    // else if (m_bfMP4 && m_bIsEOS)
    // {
    //     m_qwPosition = qwPosition;
    // }

    g_print("AMDEBUG CGSTMFByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu S_OK\n", qwPosition, m_qwPosition);

    return S_OK;
}

HRESULT CGSTMFByteStream::SetLength(QWORD qwLength)
{
    return E_NOTIMPL;
}

HRESULT CGSTMFByteStream::Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten)
{
    return E_NOTIMPL;
}

// IMFMediaEventGenerator
HRESULT CGSTMFByteStream::BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *pState)
{
    EnterCriticalSection(&m_csEventLock);

    HRESULT hr = CheckEventQueueShutdown();
    if (SUCCEEDED(hr))
    {
        hr = m_pEventQueue->BeginGetEvent(pCallback, pState);
    }

    LeaveCriticalSection(&m_csEventLock);
    return hr;
}

HRESULT CGSTMFByteStream::EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent)
{
    EnterCriticalSection(&m_csEventLock);

    HRESULT hr = CheckEventQueueShutdown();
    if (SUCCEEDED(hr))
    {
        hr = m_pEventQueue->EndGetEvent(pResult, ppEvent);
    }

    LeaveCriticalSection(&m_csEventLock);
    return hr;
}

HRESULT CGSTMFByteStream::GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent)
{
    // m_pEventQueue::GetEvent() can block, so do not hold m_csEventLock.
    IMFMediaEventQueue *pQueue = NULL;

    EnterCriticalSection(&m_csEventLock);
    HRESULT hr = CheckEventQueueShutdown();
    if (SUCCEEDED(hr))
    {
        pQueue = m_pEventQueue;
        pQueue->AddRef();
    }
    LeaveCriticalSection(&m_csEventLock);

    if (SUCCEEDED(hr))
        hr = m_pEventQueue->GetEvent(dwFlags, ppEvent);

    SafeRelease(&m_pEventQueue);
    return hr;
}

HRESULT CGSTMFByteStream::QueueEvent(MediaEventType met, REFGUID extendedType, HRESULT hrStatus, const PROPVARIANT *pvValue)
{
    EnterCriticalSection(&m_csEventLock);

    HRESULT hr = CheckEventQueueShutdown();
    if (SUCCEEDED(hr))
        hr = m_pEventQueue->QueueEventParamVar(met, extendedType, hrStatus, pvValue);

    LeaveCriticalSection(&m_csEventLock);
    return hr;
}

// IUnknown
HRESULT CGSTMFByteStream::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
    {
        return E_POINTER;
    }
    else if (riid == IID_IUnknown)
    {
        *ppvObject = static_cast<IUnknown *>(static_cast<IMFByteStream *>(this));
    }
    else if (riid == IID_IMFByteStream)
    {
        *ppvObject = static_cast<IMFByteStream *>(this);
    }
    else if (riid == IID_IMFMediaEventGenerator)
    {
        *ppvObject = static_cast<IMFMediaEventGenerator *>(this);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CGSTMFByteStream::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG CGSTMFByteStream::Release()
{
    ULONG uCount = InterlockedDecrement(&m_ulRefCount);
    if (uCount == 0)
    {
        ShutdownEventQueue();
        delete this;
    }
    return uCount;
}

// IMFMediaEventGenerator
HRESULT CGSTMFByteStream::CheckEventQueueShutdown() const
{
    return (m_bEventQueueShutdown ? MF_E_SHUTDOWN : S_OK);
}

HRESULT CGSTMFByteStream::ShutdownEventQueue()
{
    EnterCriticalSection(&m_csEventLock);

    HRESULT hr = CheckEventQueueShutdown();
    if (SUCCEEDED(hr))
    {
        if (m_pEventQueue)
            hr = m_pEventQueue->Shutdown();

        SafeRelease(&m_pEventQueue);

        m_bEventQueueShutdown = TRUE;
    }

    LeaveCriticalSection(&m_csEventLock);
    return hr;
}

HRESULT CGSTMFByteStream::ReadData()
{
    HRESULT hr = S_OK;
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstBuffer *buf = NULL;
    guint64 offset = 0;
    ULONG cbBytes = 0;

    // Read data from upstream
    do
    {
        // Prepare next segment. If we do not have segment info then query it
        // or if we read entire segment. HLSProgressBuffer will auto switch to
        // next one, so once we read it just query info for next one.
        if (m_bfMP4 && (m_qwSegmentLength == -1 || m_qwSegmentPosition >= m_qwSegmentLength))
        {
            gint64 data_length = 0;
            if (gst_pad_peer_query_duration(m_pSinkPad, GST_FORMAT_BYTES, &data_length))
                SetSegmentLength((QWORD)data_length, true);
            else
                return PrepareWaitForData(); // HLS is not ready yet, so wait for it
        }
        // If length known adjust m_cbBytes to make sure we do not read
        // pass EOS. "progressbuffer" does not handle last buffer nicely
        // and will return EOS if we do not read exact amount of data.
        else if (!m_bfMP4)
        {
            if (m_qwPosition < m_qwLength && (m_qwPosition + m_cbBytes) > m_qwLength)
                m_cbBytes = m_qwLength - m_qwPosition;
        }

        if (m_cbBytesRead < m_cbBytes)
            cbBytes = m_cbBytes - m_cbBytesRead;
        else
            return CompleteReadData(E_FAIL);

        if (m_bfMP4)
            offset = (guint64)m_qwSegmentPosition;
        else if (m_qwLength != -1)
            offset = (guint64)m_qwPosition;
        else
            return CompleteReadData(E_FAIL);

        ret = gst_pad_pull_range(m_pSinkPad, offset, (guint)cbBytes, &buf);
        if (ret == GST_FLOW_FLUSHING)
        {
            // Wait for FX_EVENT_RANGE_READY. It will be send when data available.
            return PrepareWaitForData();
        }
        else if (ret == GST_FLOW_EOS)
        {
            m_bIsEOS = TRUE;
            return CompleteReadData(S_OK);
        }
        else if (ret == GST_FLOW_OK)
        {
            hr = PushDataBuffer(buf);
            if (FAILED(hr))
                return CompleteReadData(E_FAIL);
            else if (m_cbBytesRead == m_cbBytes || m_bIsEOS)
                return CompleteReadData(S_OK);
        }
        else
        {
            return CompleteReadData(E_FAIL);
        }
    } while (SUCCEEDED(hr) && m_cbBytesRead < m_cbBytes);

    return hr;
}

HRESULT CGSTMFByteStream::PushDataBuffer(GstBuffer* pBuffer)
{
    HRESULT hr = S_OK;

    if (pBuffer == NULL)
        return E_POINTER;

    // Set EOS flag, so we can complete and signal EOS
    if (m_bIsEOSEventReceived)
        m_bIsEOS = TRUE;

    // If we received discont buffer it means format change and this buffer
    // contains new data. For example when we do bitrate switch in HLS.
    // Do not push this buffer and just ignore it. Once mfdemux reads all
    // samples it will reload MF Source Reader, so it can re-initialize for
    // new stream. hlsprogressbuffer will provide this buffer again after
    // we reload, but without discont flag, since it should be cleared once
    // provided by hlsprogressbuffer.
    if (GST_BUFFER_FLAG_IS_SET(pBuffer, GST_BUFFER_FLAG_DISCONT))
    {
        // INLINE - gst_buffer_unref()
        gst_buffer_unref(pBuffer);
        m_bIsReload = TRUE;
        m_bIsEOS = TRUE;
        m_qwLength = m_qwPosition;
        QueueEvent(MEByteStreamCharacteristicsChanged, GUID_NULL, S_OK, NULL);
        return S_OK;
    }

    GstMapInfo info;
    gboolean unmap = FALSE;
    if (gst_buffer_map(pBuffer, &info, GST_MAP_READ))
        unmap = TRUE;
    else
        hr = E_FAIL;

    if (SUCCEEDED(hr) && m_cbBytesRead >= m_cbBytes)
        hr = E_FAIL;

    if (SUCCEEDED(hr) && (m_cbBytes - m_cbBytesRead) < info.size)
        hr = E_FAIL;

    if (SUCCEEDED(hr) &&
            memcpy_s(m_pBytes + m_cbBytesRead, m_cbBytes - m_cbBytesRead,
                    info.data, info.size) != 0)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        m_cbBytesRead += info.size;
        m_qwPosition += info.size;
        m_qwSegmentPosition += info.size;
    }

    if (unmap)
        gst_buffer_unmap(pBuffer, &info);

    // INLINE - gst_buffer_unref()
    gst_buffer_unref(pBuffer);

    return hr;
}

HRESULT CGSTMFByteStream::PrepareWaitForData()
{
    Lock();
    m_bWaitForEvent = TRUE;
    Unlock();

    // In HLS mode prepare for next segment
    if (m_bfMP4)
    {
        m_qwSegmentLength = -1;
        m_qwSegmentPosition = 0;
    }

    return S_OK;
}

void CGSTMFByteStream::Lock()
{
    EnterCriticalSection(&m_csLock);
}

void CGSTMFByteStream::Unlock()
{
    LeaveCriticalSection(&m_csLock);
}