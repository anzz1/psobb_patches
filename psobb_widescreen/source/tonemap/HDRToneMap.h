// HDRToneMap.h

#ifndef HDRTONEMAP_H
#define HDRTONEMAP_H

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <dxgi.h>

extern HMODULE g_hModule;

class HDRToneMap {
    public:
        HDRToneMap(IDirect3DDevice9 *device, int width, int height);
        ~HDRToneMap();

        void go(IDirect3DTexture9 *frame, IDirect3DSurface9 *dst);

    private:
        void toneMapPass(IDirect3DTexture9* frame, IDirect3DSurface9 *dst);
        void quad(int width, int height);

        IDirect3DDevice9 *device;
        ID3DXEffect *effect;
        IDirect3DVertexDeclaration9 *vertexDeclaration;

        D3DXHANDLE frameTexHandle;

        int width, height;
};

#endif
