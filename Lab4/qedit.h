// qedit.h - DirectShow Sample Grabber interfaces
// Этот файл может отсутствовать в MinGW, поэтому создан вручную

#ifndef __qedit_h__
#define __qedit_h__

#ifdef __cplusplus
extern "C" {
#endif

// {C1F400A0-3F08-11d3-9F0B-006008039E37}
DEFINE_GUID(CLSID_SampleGrabber,
0xc1f400a0, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);

// {6B652FFF-11FE-4fce-92AD-0266B5D7C78F}
DEFINE_GUID(CLSID_NullRenderer,
0x6b652fff, 0x11fe, 0x4fce, 0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f);

// {0579154A-2B53-4994-B0D0-E773148EFF85}
DEFINE_GUID(IID_ISampleGrabberCB,
0x0579154a, 0x2b53, 0x4994, 0xb0, 0xd0, 0xe7, 0x73, 0x14, 0x8e, 0xff, 0x85);

// {6B652FFF-11FE-4fce-92AD-0266B5D7C78F}
DEFINE_GUID(IID_ISampleGrabber,
0x6b652fff, 0x11fe, 0x4fce, 0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f);

#ifdef __cplusplus
}
#endif

// ISampleGrabberCB interface
interface ISampleGrabberCB : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample *pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) = 0;
};

// ISampleGrabber interface
interface ISampleGrabber : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback) = 0;
};

#endif // __qedit_h__

