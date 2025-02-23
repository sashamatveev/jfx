/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates. All rights reserved.
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

#include "mfgstbytestream.h"

#define ENABLE_TRACE 0
#if ENABLE_TRACE
    #define TRACE g_print
#else // ENABLE_TRACE
    #define TRACE
#endif // ENABLE_TRACE

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

CMFGSTByteStream::CMFGSTByteStream(HRESULT &hr, QWORD qwLength, GstPad *pSinkPad)
{
    hr = S_OK;

    // If length is not provided we assume it is fMP4
    if (qwLength == -1)
        m_bfMP4 = TRUE;
    else
        m_bfMP4 = FALSE;

    m_ulRefCount = 0;
    m_qwPosition = 0;
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

CMFGSTByteStream::~CMFGSTByteStream()
{
    DeleteCriticalSection(&m_csLock);
    DeleteCriticalSection(&m_csEventLock);
}

void CMFGSTByteStream::Shutdown()
{
    ShutdownEventQueue();
}

HRESULT CMFGSTByteStream::ReadRangeAvailable()
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

void CMFGSTByteStream::SetSegmentLength(QWORD qwSegmentLength, bool bForce)
{
    Lock();
    if (bForce || m_bWaitForEvent)
    {
        m_qwSegmentLength = qwSegmentLength;
        m_qwSegmentPosition = 0;
    }
    Unlock();
}

// Even if we reporting MFBYTESTREAM_IS_SEEKABLE to MF to make it happy (will not
// initialized otherwise), we can only issue seek on MF source reader if length
// is known (HTTP/FILE). For HLS we wil forward seek event upstream to handle
// seek.
bool CMFGSTByteStream::IsSeekSupported()
{
    return !m_bfMP4;
}

HRESULT CMFGSTByteStream::CompleteReadData(HRESULT hr)
{
    TRACE("JFXMEDIA CMFGSTByteStream::CompleteReadData() 0x%X m_pCallback %p m_pAsyncResult %p\n", hr, m_pCallback, m_pAsyncResult);
    m_readResult = hr;
    if (m_pCallback && m_pAsyncResult)
        return m_pCallback->Invoke(m_pAsyncResult);

    return S_OK;
}

void CMFGSTByteStream::SignalEOS()
{
    m_bIsEOSEventReceived = TRUE;
}

void CMFGSTByteStream::ClearEOS()
{
    m_bIsEOS = FALSE;
    m_bIsEOSEventReceived = FALSE;
}

BOOL CMFGSTByteStream::IsReload()
{
    return m_bIsReload;
}

// IMFByteStream
HRESULT CMFGSTByteStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    HRESULT hr = S_OK;

    if (pb == NULL || pCallback == NULL)
        return E_POINTER;

    if (m_pSinkPad == NULL)
        return E_POINTER;

    if (m_readResult != S_OK)
        return m_readResult; // Do not start new read if old one failed.

    TRACE("JFXMEDIA CMFGSTByteStream::BeginRead() cb: %lu m_qwSegmentLength: %llu m_qwSegmentPosition: %llu m_qwPosition: %llu m_qwLength: %llu\n", cb, m_qwSegmentLength, m_qwSegmentPosition, m_qwPosition, m_qwLength);

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

HRESULT CMFGSTByteStream::BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    return E_NOTIMPL;
}

HRESULT CMFGSTByteStream::Close()
{
    // Nothing to close
    return S_OK;
}

HRESULT CMFGSTByteStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead)
{
    Lock();
    m_bWaitForEvent = FALSE;
    Unlock();

    if (pResult != NULL)
        pResult->SetStatus(m_readResult);

    *pcbRead = m_cbBytesRead;

    m_pCallback = NULL;
    m_pAsyncResult = NULL;

    TRACE("JFXMEDIA CMFGSTByteStream::EndRead() m_cbBytesRead: %lu\n", m_cbBytesRead);

    return S_OK;
}

HRESULT CMFGSTByteStream::EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten)
{
    return E_NOTIMPL;
}

HRESULT CMFGSTByteStream::Flush()
{
    return E_NOTIMPL;
}

HRESULT CMFGSTByteStream::GetCapabilities(DWORD *pdwCapabilities)
{
    (*pdwCapabilities) |= MFBYTESTREAM_IS_READABLE;
    (*pdwCapabilities) |= MFBYTESTREAM_IS_SEEKABLE;
    (*pdwCapabilities) |= MFBYTESTREAM_IS_REMOTE;
    return S_OK;
}

HRESULT CMFGSTByteStream::GetCurrentPosition(QWORD *pqwPosition)
{
    if (pqwPosition == NULL)
        return E_POINTER;

    (*pqwPosition) = m_qwPosition;

    TRACE("JFXMEDIA CMFGSTByteStream::GetCurrentPosition() %llu\n", (*pqwPosition));
    return S_OK;
}

HRESULT CMFGSTByteStream::GetLength(QWORD *pqwLength)
{
    if (pqwLength == NULL)
        return E_FAIL;

    if (m_bfMP4 && !m_bIsEOS)
        (*pqwLength) = -1;
    else
        (*pqwLength) = m_qwLength;

    TRACE("JFXMEDIA CMFGSTByteStream::GetLength() %llu\n", (*pqwLength));

    return S_OK;
}

HRESULT CMFGSTByteStream::IsEndOfStream(BOOL *pfEndOfStream)
{
    if (pfEndOfStream == NULL)
        return E_POINTER;

    if (m_bIsEOS)
        (*pfEndOfStream) = TRUE;
    else if (m_qwPosition >= m_qwLength)
        (*pfEndOfStream) = TRUE;
    else
        (*pfEndOfStream) = FALSE;

    TRACE("JFXMEDIA CMFGSTByteStream::IsEndOfStream() %d\n", (*pfEndOfStream));

    return S_OK;
}

HRESULT CMFGSTByteStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
    return E_NOTIMPL;
}

HRESULT CMFGSTByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition)
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

HRESULT CMFGSTByteStream::SetCurrentPosition(QWORD qwPosition)
{
    TRACE("JFXMEDIA CMFGSTByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu\n", qwPosition, m_qwPosition);
    if (qwPosition > m_qwLength)
    {
        TRACE("JFXMEDIA CMFGSTByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu E_INVALIDARG\n", qwPosition, m_qwPosition);
        return E_INVALIDARG;
    }

    if (m_qwPosition == qwPosition)
    {
        TRACE("JFXMEDIA CMFGSTByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu S_OK\n", qwPosition, m_qwPosition);
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

    TRACE("JFXMEDIA CMFGSTByteStream::SetCurrentPosition() qwPosition: %llu m_qwPosition: %llu S_OK\n", qwPosition, m_qwPosition);

    return S_OK;
}

HRESULT CMFGSTByteStream::SetLength(QWORD qwLength)
{
    return E_NOTIMPL;
}

HRESULT CMFGSTByteStream::Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten)
{
    return E_NOTIMPL;
}

// IMFMediaEventGenerator
HRESULT CMFGSTByteStream::BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *pState)
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

HRESULT CMFGSTByteStream::EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent)
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

HRESULT CMFGSTByteStream::GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent)
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

HRESULT CMFGSTByteStream::QueueEvent(MediaEventType met, REFGUID extendedType, HRESULT hrStatus, const PROPVARIANT *pvValue)
{
    EnterCriticalSection(&m_csEventLock);

    HRESULT hr = CheckEventQueueShutdown();
    if (SUCCEEDED(hr))
        hr = m_pEventQueue->QueueEventParamVar(met, extendedType, hrStatus, pvValue);

    LeaveCriticalSection(&m_csEventLock);
    return hr;
}

// IUnknown
HRESULT CMFGSTByteStream::QueryInterface(REFIID riid, void **ppvObject)
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

ULONG CMFGSTByteStream::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG CMFGSTByteStream::Release()
{
    ULONG uCount = InterlockedDecrement(&m_ulRefCount);
    if (uCount == 0)
        delete this;
    return uCount;
}

// IMFMediaEventGenerator
HRESULT CMFGSTByteStream::CheckEventQueueShutdown() const
{
    return (m_bEventQueueShutdown ? MF_E_SHUTDOWN : S_OK);
}

HRESULT CMFGSTByteStream::ShutdownEventQueue()
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

HRESULT CMFGSTByteStream::ReadData()
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

HRESULT CMFGSTByteStream::PushDataBuffer(GstBuffer* pBuffer)
{
    HRESULT hr = S_OK;

    if (pBuffer == NULL)
        return E_POINTER;

    // Set EOS flag, so we can complete and signal EOS
    if (m_bIsEOSEventReceived)
        m_bIsEOS = TRUE;

    // If we received header buffer it means format change and this buffer
    // contains new data. For example when we do bitrate switch in HLS.
    // Do not push this buffer and just ignore it. Once mfdemux reads all
    // samples it will reload MF Source Reader, so it can re-initialize for
    // new stream. hlsprogressbuffer will provide this buffer again after
    // we reload, but without header flag, since it should be cleared once
    // provided by hlsprogressbuffer.
    if (GST_BUFFER_FLAG_IS_SET(pBuffer, GST_BUFFER_FLAG_HEADER))
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

HRESULT CMFGSTByteStream::PrepareWaitForData()
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

void CMFGSTByteStream::Lock()
{
    EnterCriticalSection(&m_csLock);
}

void CMFGSTByteStream::Unlock()
{
    LeaveCriticalSection(&m_csLock);
}