// HDRToneMap.cpp

#include <sstream>
#include <vector>
#include "HDRToneMap.h"
using namespace std;

#pragma region Useful Macros from DXUT (copy-pasted here as we prefer this to be as self-contained as possible)
#if defined(DEBUG) || defined(_DEBUG)
//#pragma comment(lib,"dxerr.lib")
//int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;
//int (WINAPIV * __vsnwprintf)(wchar_t *, size_t, const wchar_t*, va_list) = _vsnwprintf;
#if defined(UNICODE) || defined(_UNICODE)
#ifndef V
#define V(x) { hr = (x); if (FAILED(hr)) { DXTrace(__FILE__, (DWORD)__LINE__, hr, L#x, true); } }
#endif
#ifndef V_RETURN
#define V_RETURN(x) { hr = (x); if (FAILED(hr)) { return DXTrace(__FILE__, (DWORD)__LINE__, hr, L#x, true); } }
#endif
#else
#ifndef V
#define V(x) { hr = (x); if (FAILED(hr)) { DXTrace(__FILE__, (DWORD)__LINE__, hr, #x, true); } }
#endif
#ifndef V_RETURN
#define V_RETURN(x) { hr = (x); if (FAILED(hr)) { return DXTrace(__FILE__, (DWORD)__LINE__, hr, #x, true); } }
#endif
#endif
#else
#ifndef V
#define V(x) { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x) { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if (p) { delete (p); (p) = nullptr; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p); (p) = nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = nullptr; } }
#endif
#pragma endregion

#pragma region This stuff is for loading headers from resources
class ID3D10IncludeResource : public ID3DXInclude {
    public:
        STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE, LPCSTR pFileName, LPCVOID, LPCVOID *ppData, UINT *pBytes)  {
            HRSRC src = FindResourceA(g_hModule, pFileName, RT_RCDATA);
            HGLOBAL res = LoadResource(g_hModule, src);

            *pBytes = SizeofResource(g_hModule, src);
            *ppData = (LPCVOID)LockResource(res);

            return S_OK;
        }

        STDMETHOD(Close)(THIS_ LPCVOID)  {
            return S_OK;
        }
};
#pragma endregion


HDRToneMap::HDRToneMap(IDirect3DDevice9 *device, int width, int height)
        : device(device),
          width(width), height(height) {
    HRESULT hr;

		// Setup the defines for compiling the effect.
		vector<D3DXMACRO> defines;

		D3DXMACRO null = { nullptr, nullptr };
		defines.push_back(null);

		// Setup the flags for the effect.
		DWORD flags = D3DXFX_NOT_CLONEABLE | D3DXSHADER_OPTIMIZATION_LEVEL3;
#ifdef D3DXFX_LARGEADDRESS_HANDLE
		flags |= D3DXFX_LARGEADDRESSAWARE;
#endif

		/**
		 * IMPORTANT! Here we load and compile the HDRToneMap effect from a *RESOURCE*
		 * (Yeah, we like all-in-one executables for demos =)
		 * In case you want it to be loaded from other place change this line accordingly.
		 */
		ID3D10IncludeResource includeResource;
		V(D3DXCreateEffectFromResourceA(device, g_hModule, "HDRToneMap.fx", &defines.front(), &includeResource, flags, nullptr, &effect, nullptr));
		//V(D3DXCreateEffectFromFileW(device, L"HDRToneMap.fx", &defines.front(), nullptr, flags, nullptr, &effect, nullptr));

		// Vertex declaration for rendering the typical fullscreen quad later on.
		const D3DVERTEXELEMENT9 vertexElements[3] = {
				{ 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
				{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
				D3DDECL_END()
		};
		V(device->CreateVertexDeclaration(vertexElements, &vertexDeclaration));

		// Get handles
		frameTexHandle = effect->GetParameterByName(NULL, "frameTex2D");
}


HDRToneMap::~HDRToneMap() {
    SAFE_RELEASE(effect);
    SAFE_RELEASE(vertexDeclaration);
}

void HDRToneMap::go(IDirect3DTexture9 *frame, IDirect3DSurface9 *dst) {
    HRESULT hr;

    // Setup the layout for our fullscreen quad.
    V(device->SetVertexDeclaration(vertexDeclaration));

    // And here we go!
    toneMapPass(frame, dst);
}

void HDRToneMap::toneMapPass(IDirect3DTexture9* frame, IDirect3DSurface9 *dst) {
    D3DPERF_BeginEvent(D3DCOLOR_XRGB(0, 0, 0), L"HDRToneMap: 1st pass");
    HRESULT hr;

    // Set the render target and clear both the color and the stencil buffers.
    V(device->SetRenderTarget(0, dst));
    V(device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0));

    // Setup variables.
    V(effect->SetTexture(frameTexHandle, frame));

    // Do it!
    UINT passes;
    V(effect->Begin(&passes, 0));
    V(effect->BeginPass(0));
    quad(width, height);
    V(effect->EndPass());
    V(effect->End());

    D3DPERF_EndEvent();
}

void HDRToneMap::quad(int width, int height) {
    // Typical aligned fullscreen quad.
    HRESULT hr;
    D3DXVECTOR2 pixelSize = D3DXVECTOR2(1.0f / float(width), 1.0f / float(height));
    float quad[4][5] = {
        { -1.0f - pixelSize.x,  1.0f + pixelSize.y, 0.5f, 0.0f, 0.0f },
        {  1.0f - pixelSize.x,  1.0f + pixelSize.y, 0.5f, 1.0f, 0.0f },
        { -1.0f - pixelSize.x, -1.0f + pixelSize.y, 0.5f, 0.0f, 1.0f },
        {  1.0f - pixelSize.x, -1.0f + pixelSize.y, 0.5f, 1.0f, 1.0f }
    };
    V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0])));
}
