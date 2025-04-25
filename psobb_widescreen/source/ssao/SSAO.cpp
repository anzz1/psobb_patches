// SSAO.cpp

#include <sstream>
#include <vector>
#include "SSAO.h"
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


SSAO::SSAO(IDirect3DDevice9 *device, int width, int height)
        : device(device),
          width(width), height(height) {
    HRESULT hr;

		// Setup the defines for compiling the effect.
		vector<D3DXMACRO> defines;
		stringstream s;

		// Setup pixel size macro
		s << "float2(1.0 / " << width << ", 1.0 / " << height << ")";
		string pixelSizeText = s.str();
		D3DXMACRO pixelSizeMacro = { "PIXEL_SIZE", pixelSizeText.c_str() };
		defines.push_back(pixelSizeMacro);

		// Setup screen size macro
		stringstream().swap(s);
		s << "float2(" << width << ", " << height << ")";
		string screenSizeText = s.str();
		D3DXMACRO screenSizeMacro = { "SCREEN_SIZE", screenSizeText.c_str() };
		defines.push_back(screenSizeMacro);

    // Setup sRGB macro
    D3DXMACRO useSrgbMacro = { "USE_SRGB", "false" };
    defines.push_back(useSrgbMacro);

		//D3DXMACRO showSsaoMacro = { "SHOW_SSAO", "1" };
		//defines.push_back(showSsaoMacro);

		D3DXMACRO null = { nullptr, nullptr };
		defines.push_back(null);

		// Setup the flags for the effect.
		DWORD flags = D3DXFX_NOT_CLONEABLE | D3DXSHADER_OPTIMIZATION_LEVEL3;
#ifdef D3DXFX_LARGEADDRESS_HANDLE
		flags |= D3DXFX_LARGEADDRESSAWARE;
#endif

		/**
		 * IMPORTANT! Here we load and compile the SSAO effect from a *RESOURCE*
		 * (Yeah, we like all-in-one executables for demos =)
		 * In case you want it to be loaded from other place change this line accordingly.
		 */
		ID3D10IncludeResource includeResource;
		V(D3DXCreateEffectFromResourceA(device, g_hModule, "SSAO.fx", &defines.front(), &includeResource, flags, nullptr, &effect, nullptr));
		//V(D3DXCreateEffectFromFileW(device, L"SSAO.fx", &defines.front(), nullptr, flags, nullptr, &effect, nullptr));

		// Vertex declaration for rendering the typical fullscreen quad later on.
		const D3DVERTEXELEMENT9 vertexElements[3] = {
				{ 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
				{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
				D3DDECL_END()
		};
		V(device->CreateVertexDeclaration(vertexElements, &vertexDeclaration));

		// Create buffers
		V(device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &buffer1Tex, NULL));
		buffer1Tex->GetSurfaceLevel(0, &buffer1Surf);
		V(device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &buffer2Tex, NULL));
		buffer2Tex->GetSurfaceLevel(0, &buffer2Surf);

		// Get handles
		depthTexHandle = effect->GetParameterByName(NULL, "depthTex2D");
		frameTexHandle = effect->GetParameterByName(NULL, "frameTex2D");
		prevPassTexHandle = effect->GetParameterByName(NULL, "prevPassTex2D");
}


SSAO::~SSAO() {
    SAFE_RELEASE(effect);
    SAFE_RELEASE(vertexDeclaration);
    SAFE_RELEASE(buffer1Surf);
    SAFE_RELEASE(buffer1Tex);
    SAFE_RELEASE(buffer2Surf);
    SAFE_RELEASE(buffer2Tex);
}

void SSAO::go(IDirect3DTexture9 *frame,
              IDirect3DTexture9 *depth,
              IDirect3DSurface9 *dst) {
    HRESULT hr;

		// Setup the layout for our fullscreen quad.
		V(device->SetVertexDeclaration(vertexDeclaration));

		// And here we go!
		mainSsaoPass(depth, buffer1Surf);
		hBlurPass(buffer1Tex, depth, buffer2Surf);
		vBlurPass(buffer2Tex, depth, buffer1Surf);
		combinePass(frame, buffer1Tex, dst);
}

void SSAO::mainSsaoPass(IDirect3DTexture9 *depth, IDirect3DSurface9 *dst) {
    D3DPERF_BeginEvent(D3DCOLOR_XRGB(0, 0, 0), L"SSAO: 1st pass");
    HRESULT hr;

    // Set the render target and clear both the color and the stencil buffers.
    V(device->SetRenderTarget(0, dst));
    V(device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 1.0f, 0));

    // Setup variables.
    V(effect->SetTexture(depthTexHandle, depth));

    // Do it!
    UINT passes;
    V(effect->Begin(&passes, 0));
    V(effect->BeginPass(0));
    quad(width, height);
    V(effect->EndPass());
    V(effect->End());

    D3DPERF_EndEvent();
}

void SSAO::hBlurPass(IDirect3DTexture9* src, IDirect3DTexture9 *depth, IDirect3DSurface9* dst) {
  D3DPERF_BeginEvent(D3DCOLOR_XRGB(0, 0, 0), L"SSAO: 2nd pass");
  HRESULT hr;

  // Set the render target
  V(device->SetRenderTarget(0, dst));

  // Setup variables.
  V(effect->SetTexture(prevPassTexHandle, src));
  V(effect->SetTexture(depthTexHandle, depth));

  // Do it!
  UINT passes;
  V(effect->Begin(&passes, 0));
  V(effect->BeginPass(1));
  quad(width, height);
  V(effect->EndPass());
  V(effect->End());

  D3DPERF_EndEvent();
}

void SSAO::vBlurPass(IDirect3DTexture9* src, IDirect3DTexture9 *depth, IDirect3DSurface9* dst) {
  D3DPERF_BeginEvent(D3DCOLOR_XRGB(0, 0, 0), L"SSAO: 3rd pass");
  HRESULT hr;

  // Set the render target
  V(device->SetRenderTarget(0, dst));

  // Setup variables.
  V(effect->SetTexture(prevPassTexHandle, src));
  V(effect->SetTexture(depthTexHandle, depth));

  // Do it!
  UINT passes;
  V(effect->Begin(&passes, 0));
  V(effect->BeginPass(2));
  quad(width, height);
  V(effect->EndPass());
  V(effect->End());

  D3DPERF_EndEvent();
}

void SSAO::combinePass(IDirect3DTexture9* frame, IDirect3DTexture9* ao, IDirect3DSurface9* dst) {
  D3DPERF_BeginEvent(D3DCOLOR_XRGB(0, 0, 0), L"SSAO: 4th pass");
  HRESULT hr;

  // Set the render target
  V(device->SetRenderTarget(0, dst));

  // Setup variables.
  V(effect->SetTexture(prevPassTexHandle, ao));
  V(effect->SetTexture(frameTexHandle, frame));

  // Do it!
  UINT passes;
  V(effect->Begin(&passes, 0));
  V(effect->BeginPass(3));
  quad(width, height);
  V(effect->EndPass());
  V(effect->End());

  D3DPERF_EndEvent();
}

void SSAO::quad(int width, int height) {
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
