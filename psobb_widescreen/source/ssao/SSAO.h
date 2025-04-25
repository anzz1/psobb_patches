// SSAO.h

#ifndef SSAO_H
#define SSAO_H

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <dxgi.h>

extern HMODULE g_hModule;

class SSAO {
    public:
        SSAO(IDirect3DDevice9 *device, int width, int height);
        ~SSAO();

        void go(IDirect3DTexture9 *frame, IDirect3DTexture9* depth, IDirect3DSurface9 *dst);

    private:
        void mainSsaoPass(IDirect3DTexture9* depth, IDirect3DSurface9 *dst);
        void hBlurPass(IDirect3DTexture9* src, IDirect3DTexture9* depth, IDirect3DSurface9* dst);
        void vBlurPass(IDirect3DTexture9* src, IDirect3DTexture9* depth, IDirect3DSurface9* dst);
        void combinePass(IDirect3DTexture9* frame, IDirect3DTexture9* ao, IDirect3DSurface9* dst);
        void quad(int width, int height);

        IDirect3D9 *d3d;
        IDirect3DDevice9 *device;
        ID3DXEffect *effect;
        IDirect3DVertexDeclaration9 *vertexDeclaration;

        IDirect3DTexture9* buffer1Tex;
        IDirect3DSurface9* buffer1Surf;
        IDirect3DTexture9* buffer2Tex;
        IDirect3DSurface9* buffer2Surf;

        D3DXHANDLE frameTexHandle;
        D3DXHANDLE depthTexHandle;
        D3DXHANDLE prevPassTexHandle;

        int width, height;
};

#endif
