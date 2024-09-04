#include "gstmfbytestream.h"

CGSTMFByteStream::CGSTMFByteStream(QWORD qwLength)
{
    m_ulRefCount = 0;
    m_qwPosition = 0;
    m_qwLength = qwLength;
}

// IMFByteStream
HRESULT CGSTMFByteStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    return E_FAIL;
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
    return E_FAIL;
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
    return E_FAIL;
}

HRESULT CGSTMFByteStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition)
{
    return E_FAIL;
}

HRESULT CGSTMFByteStream::SetCurrentPosition(QWORD qwPosition)
{
    if (qwPosition > m_qwLength)
        return E_INVALIDARG;

    if (m_qwPosition == qwPosition)
        return S_OK;

    // TODO: do seek

    m_qwPosition == qwPosition;

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
