// DOF.h

#ifndef DOF_H
#define DOF_H

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <dxgi.h>
#include "../ssao/SSAO.h"

extern HMODULE g_hModule;

class DOF {
    public:
        DOF(IDirect3DDevice9 *device, int width, int height);
        ~DOF();

        void go(IDirect3DTexture9 *frame, IDirect3DTexture9 *depth, IDirect3DSurface9 *dst);

    private:
        void dofPass(IDirect3DTexture9* frame, IDirect3DTexture9 *depth, IDirect3DSurface9 *dst);
        void quad(int width, int height);

        IDirect3DDevice9 *device;
        ID3DXEffect *effect;
        IDirect3DVertexDeclaration9 *vertexDeclaration;

        D3DXHANDLE frameTexHandle;
        D3DXHANDLE depthTexHandle;

        D3DXHANDLE dofAmtHandle;

        int width, height;
};

#endif
