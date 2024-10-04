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

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

CGSTMFByteStream::CGSTMFByteStream(QWORD qwLength, GstPad *pSinkPad)
{
    m_ulRefCount = 0;
    m_qwPosition = 0;
    m_qwLength = qwLength;

    m_pBytes = NULL;
    m_cbBytes = 0;
    m_pCallback = NULL;
    m_pAsyncResult = NULL;
    m_bWaitForEvent = FALSE;

    m_pSinkPad = pSinkPad;

    InitializeCriticalSection(&m_csLock);
}

CGSTMFByteStream::~CGSTMFByteStream()
{
    ::DeleteCriticalSection(&m_csLock);
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

// IMFByteStream
HRESULT CGSTMFByteStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    if (pb == NULL || pCallback == NULL)
        return E_POINTER;

    if (m_pSinkPad == NULL)
        return E_POINTER;

    m_pBytes = pb;
    m_cbBytes = (cb < m_qwLength) ? cb : m_qwLength;
    m_pCallback = pCallback;

    HRESULT hr = MFCreateAsyncResult(NULL, pCallback, punkState, &m_pAsyncResult);
    if (FAILED(hr))
        return hr;

    return ReadData();
}

HRESULT CGSTMFByteStream::BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::Close()
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead)
{
    Lock();
    m_bWaitForEvent = FALSE;
    Unlock();

    SafeRelease(&m_pAsyncResult);

    *pcbRead = m_cbBytes;

    return S_OK;
}

HRESULT CGSTMFByteStream::EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten)
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::Flush()
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::GetCapabilities(DWORD *pdwCapabilities)
{
    // TODO set caps based on real information
    (*pdwCapabilities) |= MFBYTESTREAM_IS_READABLE;
    (*pdwCapabilities) |= MFBYTESTREAM_IS_SEEKABLE;
    (*pdwCapabilities) |= MFBYTESTREAM_IS_REMOTE;
    return S_OK;
}

HRESULT CGSTMFByteStream::GetCurrentPosition(QWORD *pqwPosition)
{
    (*pqwPosition) = m_qwPosition;
    return S_OK;
}

HRESULT CGSTMFByteStream::GetLength(QWORD *pqwLength)
{
    (*pqwLength) = m_qwLength;
    return S_OK;
}

HRESULT CGSTMFByteStream::IsEndOfStream(BOOL *pfEndOfStream)
{
    if (m_qwPosition >= m_qwLength)
        *pfEndOfStream = TRUE;
    else
        *pfEndOfStream = FALSE;

    return S_OK;
}

HRESULT CGSTMFByteStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
    return E_FAIL;
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
    if (qwPosition > m_qwLength)
        return E_INVALIDARG;

    if (m_qwPosition == qwPosition)
        return S_OK;

    m_qwPosition = qwPosition;

    return S_OK;
}

HRESULT CGSTMFByteStream::SetLength(QWORD qwLength)
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten)
{
    return E_FAIL;
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
        *ppvObject = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_IMFByteStream)
    {
        *ppvObject = static_cast<IMFByteStream *>(this);
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
        delete this;
    }
    return uCount;
}

HRESULT CGSTMFByteStream::ReadData()
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstBuffer *buf = NULL;
    guint64 offset = (guint64)m_qwPosition;
    guint size = (guint)m_cbBytes;

    if (m_pAsyncResult == NULL)
        return E_INVALIDARG;

    // Read data from upstream
    ret = gst_pad_pull_range(m_pSinkPad, offset, size, &buf);
    if (ret == GST_FLOW_FLUSHING)
    {
        // Wait for FX_EVENT_RANGE_READY. It will be send when data available.
        Lock();
        m_bWaitForEvent = TRUE;
        Unlock();
        return S_OK;
    }
    else if (ret == GST_FLOW_OK)
    {
        GstMapInfo info;
        if (!gst_buffer_map(buf, &info, GST_MAP_READ))
        {
            // INLINE - gst_buffer_unref()
            gst_buffer_unref(buf);
            return E_FAIL;
        }

        memcpy(m_pBytes, info.data, m_cbBytes);

        gst_buffer_unmap(buf, &info);

        // INLINE - gst_buffer_unref()
        gst_buffer_unref(buf);

        m_qwPosition += m_cbBytes;

        return m_pCallback->Invoke(m_pAsyncResult);
    }
    else
    {
        m_cbBytes = 0;
        return m_pCallback->Invoke(m_pAsyncResult);
    }

    return E_FAIL;
}

void CGSTMFByteStream::Lock()
{
    ::EnterCriticalSection(&m_csLock);
}

void CGSTMFByteStream::Unlock()
{
    ::LeaveCriticalSection(&m_csLock);
}