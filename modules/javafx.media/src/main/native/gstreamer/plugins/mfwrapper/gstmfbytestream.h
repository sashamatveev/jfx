#ifndef __GST_MF_BYTESTREAM_H__
#define __GST_MF_BYTESTREAM_H__

#include <gst/gst.h>

#include <mfapi.h>
#include <mfobjects.h>

class CGSTMFByteStream : public IMFByteStream
{
public:
    CGSTMFByteStream(QWORD qwLength, GstPad *pSinkPad);

    HRESULT ReadRangeAvailable();

    // IMFByteStream
    HRESULT BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
    HRESULT BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
    HRESULT Close();
    HRESULT EndRead(IMFAsyncResult *pResult, ULONG *pcbRead);
    HRESULT EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten);
    HRESULT Flush();
    HRESULT GetCapabilities(DWORD *pdwCapabilities);
    HRESULT GetCurrentPosition(QWORD *pqwPosition);
    HRESULT GetLength(QWORD *pqwLength);
    HRESULT IsEndOfStream(BOOL *pfEndOfStream);
    HRESULT Read(BYTE *pb, ULONG cb, ULONG *pcbRead);
    HRESULT Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition);
    HRESULT SetCurrentPosition(QWORD qwPosition);
    HRESULT SetLength(QWORD qwLength);
    HRESULT Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten);

    // IUnknown
    HRESULT QueryInterface(REFIID riid, void **ppvObject);
    ULONG AddRef();
    ULONG Release();

private:
    HRESULT ReadData();

private:
    ULONG m_ulRefCount;

    QWORD m_qwPosition;
    QWORD m_qwLength;

    // Read
    BYTE *m_pBytes;
    ULONG m_cbBytes;
    IMFAsyncCallback *m_pCallback;
    IMFAsyncResult   *m_pAsyncResult;

    GstPad *m_pSinkPad;
};

#endif // __GST_MF_BYTESTREAM_H__
