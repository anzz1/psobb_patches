//-----------------------------------------------------------------------------
// author: Dmytro Shchukin
//-----------------------------------------------------------------------------
#include "DepthTexture.hpp"
#include <nvapi.h>

#define FOURCC_RESZ ((D3DFORMAT)(MAKEFOURCC('R','E','S','Z')))
#define FOURCC_INTZ ((D3DFORMAT)(MAKEFOURCC('I','N','T','Z')))
#define FOURCC_RAWZ ((D3DFORMAT)(MAKEFOURCC('R','A','W','Z')))
#define RESZ_CODE 0x7fa05000

//--------------------------------------------------------------------------------------
DepthTexture::DepthTexture(const LPDIRECT3D9 d3d)
	: m_registeredDSS( NULL )
{
	m_pTexture = NULL;
	D3DDISPLAYMODE currentDisplayMode;
	d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &currentDisplayMode);

	// determine if RESZ is supported
	m_isRESZ = d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		currentDisplayMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, FOURCC_RESZ ) == D3D_OK;

	// determine if INTZ is supported
	m_isINTZ = d3d->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		currentDisplayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, FOURCC_INTZ ) == D3D_OK;

	// determine if RAWZ is supported, used in GeForce 6-7 series.
	m_isRAWZ = d3d->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		currentDisplayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, FOURCC_RAWZ ) == D3D_OK;

	// determine if RESZ or NVAPI supported
	m_isSupported = ( NvAPI_Initialize() == NVAPI_OK || m_isRESZ ) && ( m_isRAWZ || m_isINTZ );
}

//--------------------------------------------------------------------------------------
void DepthTexture::createTexture( const LPDIRECT3DDEVICE9 device, int width, int height )
{
	if (m_isSupported)
	{
		D3DFORMAT format = m_isINTZ ? FOURCC_INTZ : FOURCC_RAWZ;

		device->CreateTexture(width, height, 1,
			D3DUSAGE_DEPTHSTENCIL, format,
			D3DPOOL_DEFAULT, &m_pTexture,
			NULL);

		if (!m_isRESZ)
		{
			NvAPI_D3D9_RegisterResource(m_pTexture);
		}
	}
}

//--------------------------------------------------------------------------------------
DepthTexture::~DepthTexture()
{
	if (m_pTexture)
	{
		if (!m_isRESZ)
		{
			NvAPI_D3D9_UnregisterResource(m_pTexture);
		}
		m_pTexture->Release();
	}
	if (m_registeredDSS != NULL)
	{
		NvAPI_D3D9_UnregisterResource(m_registeredDSS);
	}
}

//--------------------------------------------------------------------------------------
void DepthTexture::resolveDepth(const LPDIRECT3DDEVICE9 device, IDirect3DSurface9* depth)
{
	if (m_isRESZ)
	{
		device->SetVertexShader(NULL);
		device->SetPixelShader(NULL);
		device->SetFVF(D3DFVF_XYZ);
		// Bind depth stencil texture to texture sampler 0
		device->SetTexture(0, m_pTexture);
		// Perform a dummy draw call to ensure texture sampler 0 is set before the // resolve is triggered
		// Vertex declaration and shaders may need to me adjusted to ensure no debug
		// error message is produced
		D3DXVECTOR3 vDummyPoint(0.0f, 0.0f, 0.0f);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		device->DrawPrimitiveUP(D3DPT_POINTLIST, 1, vDummyPoint, sizeof(D3DXVECTOR3));
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

		// Trigger the depth buffer resolve; after this call texture sampler 0
		// will contain the contents of the resolve operation
		device->SetRenderState(D3DRS_POINTSIZE, RESZ_CODE);

		// This hack to fix resz hack, has been found by Maksym Bezus!!!
		// Without this line resz will be resolved only for first frame
		device->SetRenderState(D3DRS_POINTSIZE, 0); // TROLOLO!!!
	}
	else
	{
		if (m_registeredDSS != depth) {
			NvAPI_D3D9_RegisterResource(depth);
			if (m_registeredDSS != NULL) {
				// Unregister old one if there is any
				NvAPI_D3D9_UnregisterResource(m_registeredDSS);
			}
			m_registeredDSS = depth;
		}
		NvAPI_D3D9_StretchRectEx(device, depth, NULL, m_pTexture, NULL, D3DTEXF_LINEAR);
	}
}