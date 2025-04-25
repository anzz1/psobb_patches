/*
SSAO GLSL shader v1.3
assembled by Martins Upitis (martinsh) (devlog-martinsh.blogspot.com)
original technique is made by Arkano22 (www.gamedev.net/topic/550699-ssao-no-halo-artifacts/)

changelog:
1.3 - heavy optimization - varying sample count and ao density areas based on luminance
1.2 - added fog calculation to mask AO. Minor fixes.
1.1 - added spiral sampling method from here:
(http://www.cgafaq.info/wiki/Evenly_distributed_points_on_sphere)
*/

//------------------------------------------
// TWEAKABLE VALUES

//#define SHOW_SSAO

#define SCALE 1.0

#define LUMINANCE_CONSIDERATION
float luminosity_threshold = 0.3;

//make sure that these two values are the same for your camera, otherwise distances will be wrong.

float nearZ = 0.004; //Z-near
float farZ = 1.0; //Z-far

//user variables
int samples = 16; //ao sample count

float radius = 12.0; //ao radius
bool noise = false; //use noise instead of pattern for sample dithering
float noiseamount = 0.0002; //dithering amount

float diffarea = 0.2; //self-shadowing reduction
float gdisplace = 0.4; //gauss bell center
float aoWidth = 2.0; //gauss bell width

bool mist = true; //use mist?
float miststart = 0.05; //mist start
float mistend = 0.7; //mist end

bool optimize = true;
int minsamples = 2;

float aoClamp = 0.1; //depth clamp - reduces haloing at screen edges

// END OF TWEAKABLE VALUES
//--------------------------------------------------------

#ifndef PIXEL_SIZE // This is just for compilation-time syntax checking.
#define PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
#endif

#ifndef SCREEN_SIZE // This is just for compilation-time syntax checking.
#define SCREEN_SIZE float2(1280.0, 720.0)
#endif

#ifndef USE_SRGB
#define USE_SRGB true
#endif

#define PI    3.14159265

static float2 rcpres = PIXEL_SIZE;
float width  = SCREEN_SIZE.x;
float height = SCREEN_SIZE.y;


texture2D depthTex2D;
sampler depthSampler = sampler_state
{
	texture = <depthTex2D>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = POINT;
	AddressU  = Mirror;
	AddressV  = Mirror;
	SRGBTexture = FALSE;
};

texture2D frameTex2D;
sampler frameSampler = sampler_state
{
	texture = <frameTex2D>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture = USE_SRGB;
};

texture2D prevPassTex2D;
sampler passSampler = sampler_state
{
	texture = <prevPassTex2D>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture = FALSE;
};

struct VSOUT
{
	float4 vertPos : POSITION0;
	float2 UVCoord : TEXCOORD0;
};

struct VSIN
{
	float4 vertPos : POSITION0;
	float2 UVCoord : TEXCOORD0;
};


VSOUT FrameVS(VSIN IN)
{
	VSOUT OUT;
	float4 pos=float4(IN.vertPos.x, IN.vertPos.y, IN.vertPos.z, 1.0f);
	OUT.vertPos=pos;
	float2 coord=float2(IN.UVCoord.x, IN.UVCoord.y);
	OUT.UVCoord=coord;
	return OUT;
}

float2 rand(in float2 coord : TEXCOORD0) // generating noise/pattern texture for dithering
{
	float noiseX = ((frac(1.0-coord.x*(width/2.0))*0.25)+(frac(coord.y*(height/2.0))*0.75))*2.0-1.0;
	float noiseY = ((frac(1.0-coord.x*(width/2.0))*0.75)+(frac(coord.y*(height/2.0))*0.25))*2.0-1.0;
	
	if (noise)
	{
		noiseX = clamp(frac(sin(dot(coord,float2(12.9898,78.233))) * 43758.5453),0.0,1.0)*2.0-1.0;
		noiseY = clamp(frac(sin(dot(coord,float2(12.9898,78.233)*2.0)) * 43758.5453),0.0,1.0)*2.0-1.0;
	}
	return float2(noiseX,noiseY)*2.0;
}

float doMist(in float2 coord : TEXCOORD0) 
{
	float z = tex2Dlod(depthSampler, float4(coord, 0, 0)).r;
	float depth = -farZ * nearZ / (z * (farZ - nearZ) - farZ);
	return clamp((depth - miststart) / mistend, 0.0, 1.0);
}

float readColor(in float2 coord : TEXCOORD0)
{
    return tex2D(frameSampler, coord).rgb;
}


float readDepth(in float2 coord : TEXCOORD0)
{
    return (2.0f * nearZ) / ((farZ + nearZ) - tex2Dlod(depthSampler, float4(coord, 0, 0)).r * (farZ - nearZ));
}

float compareDepths(in float depth1, in float depth2, inout int far)
{   
	float garea = aoWidth; //gauss bell width    
	float diff = (depth1 - depth2) * 100.0; //depth difference (0-100)
	//reduce left bell width to avoid self-shadowing 
	if (diff < gdisplace)
	{
		garea = diffarea;
	} else {
		far = 1;
	}

	float gauss = pow(2.7182,-2.0*(diff-gdisplace)*(diff-gdisplace)/(garea*garea));
	return gauss;
}   

float calAO(float2 tex, float depth, float dw, float dh)
{
	//float dd = (1.0-depth)*radius;
	float dd = radius - depth;	
	float temp = 0.0;
	float temp2 = 0.0;
	float coordw = tex.x + dw*dd;
	float coordh = tex.y + dh*dd;
	float coordw2 = tex.x - dw*dd;
	float coordh2 = tex.y - dh*dd;
	
	float2 coord = float2(coordw, coordh);
	float2 coord2 = float2(coordw2, coordh2);
	
	int far = 0;
	temp = compareDepths(depth, readDepth(coord),far);
	//DEPTH EXTRAPOLATION:
	if (far > 0)
	{
		temp2 = compareDepths(readDepth(coord2),depth,far);
		temp += (1.0-temp)*temp2;
	}
	
	return temp;
} 

float4 ssao_Main(VSOUT IN) : COLOR0
{
	float2 noise = rand(IN.UVCoord); 
	float depth = readDepth(IN.UVCoord);

	float fog = doMist(IN.UVCoord);	

    float3 color = tex2D(frameSampler, IN.UVCoord).rgb;
    float3 lumcoeff = float3(0.299,0.587,0.114);
    float lum = dot(color.rgb, lumcoeff);
    float luminance = float3(lum, lum, lum);

    float w = (1.0 / width)/clamp(depth,aoClamp,1.0)*noise.x;
    float h = (1.0 / height)/clamp(depth,aoClamp,1.0)*noise.y;
	
	float pw;
	float ph;
	
	float ao = 0;

    int samp = samples;

    if (optimize)
    {
		samp = float(minsamples)+clamp(1.0-(fog+pow(lum*1.0,4.0)),0.0,1.0)*(float(samples)-float(minsamples));
    }
	
	float dl = PI*(3.0-sqrt(5.0));
	float dz = 1.0/float(samp);
	float l = 0.0;
	float z = 1.0 - dz/2.0;
	
	for (int i = 0; i <= samp; i ++)
	{     
		float r = sqrt(1.0-z);
		pw = cos(l)*r*(1.0-doMist(IN.UVCoord));
         ph = sin(l)*r*(1.0-doMist(IN.UVCoord));
		ao += calAO(IN.UVCoord,depth,pw*w,ph*h);
		z = z - dz;
		l = l + dl;
	}
	
	ao /= float(samp) + 0.1;
	ao = 1.0 - ao;
	
	if (mist)
	{
		ao = lerp(ao, 1.0, fog);
	}

	return float4(float3(ao, ao, ao),1.0);
}

#ifdef BLUR_SHARP
#define THICKNESS_FACTOR 0.2
float4 HBlur( VSOUT IN ) : COLOR0 {
	float4 sample = tex2D(passSampler, IN.UVCoord);
	#ifdef DONT_BLUR
	return sample;
	#endif
	float blurred = sample.r*2;
	float depth = sample.a;
	float divide = 2.0;

	float4 left = tex2D(passSampler, IN.UVCoord - float2(rcpres.x, 0));
	float lpower = saturate((THICKNESS_FACTOR*ThicknessModel - abs(left.a - depth))*(1 / ThicknessModel));
	blurred += lpower*left.r;
	divide += lpower;

	float4 right = tex2D(passSampler, IN.UVCoord + float2(rcpres.x, 0));
	float rpower = saturate((THICKNESS_FACTOR*ThicknessModel - abs(right.a - depth))*(1 / ThicknessModel));
	blurred += rpower*left.r;
	divide += rpower;

	blurred /= divide;

	return float4(blurred, blurred, blurred, depth);
}
float4 VBlur( VSOUT IN ) : COLOR0 {
	float4 sample = tex2D(passSampler, IN.UVCoord);
	#ifdef DONT_BLUR
	return sample;
	#endif
	float blurred = sample.r*2;
	float depth = sample.a;
	float divide = 2.0;

	float4 top = tex2D(passSampler, IN.UVCoord - float2(0, rcpres.y));
	float tpower = saturate((THICKNESS_FACTOR*ThicknessModel - abs(top.a - depth))*(1 / ThicknessModel));
	blurred += tpower*top.r;
	divide += tpower;

	float4 bottom = tex2D(passSampler, IN.UVCoord + float2(0, rcpres.y));
	float bpower = saturate((THICKNESS_FACTOR*ThicknessModel - abs(bottom.a - depth))*(1 / ThicknessModel));
	blurred += bpower*bottom.r;
	divide += bpower;

	blurred /= divide;

	return float4(blurred, blurred, blurred, depth);
}
#else // BLUR_GAUSSIAN
float4 HBlur(VSOUT IN) : COLOR0 {
	float4 sample = tex2D(passSampler, IN.UVCoord);
	#ifdef DONT_BLUR
	return sample;
	#endif
	float color = sample.r;

	float blurred = color*0.2270270270;
	blurred += tex2D(passSampler, IN.UVCoord + float2(rcpres.x*1.3846153846, 0)).r * 0.3162162162;
	blurred += tex2D(passSampler, IN.UVCoord - float2(rcpres.x*1.3846153846, 0)).r * 0.3162162162;
	blurred += tex2D(passSampler, IN.UVCoord + float2(rcpres.x*3.2307692308, 0)).r * 0.0702702703;
	blurred += tex2D(passSampler, IN.UVCoord - float2(rcpres.x*3.2307692308, 0)).r * 0.0702702703;

	return float4(blurred, blurred, blurred, sample.a);
}

float4 VBlur(VSOUT IN) : COLOR0 {
	float4 sample = tex2D(passSampler, IN.UVCoord);
	#ifdef DONT_BLUR
	return sample;
	#endif
	float color = sample.r;

	float blurred = color*0.2270270270;
	blurred += tex2D(passSampler, IN.UVCoord + float2(0, rcpres.y*1.3846153846)).r * 0.3162162162;
	blurred += tex2D(passSampler, IN.UVCoord - float2(0, rcpres.y*1.3846153846)).r * 0.3162162162;
	blurred += tex2D(passSampler, IN.UVCoord + float2(0, rcpres.y*3.2307692308)).r * 0.0702702703;
	blurred += tex2D(passSampler, IN.UVCoord - float2(0, rcpres.y*3.2307692308)).r * 0.0702702703;

	return float4(blurred, blurred, blurred, sample.a);
}
#endif // blur type

float4 Combine( VSOUT IN ) : COLOR0 {
	float4 color = tex2D(frameSampler, IN.UVCoord);
	float4 aoSample = tex2D(passSampler, IN.UVCoord/SCALE);
	float ao = clamp(aoSample.r, aoClamp, 1.0);

	#ifdef LUMINANCE_CONSIDERATION
	float luminance = (color.r*0.2125f)+(color.g*0.7154f)+(color.b*0.0721f);
	float white = 1.0f;
	float black = 0.0f;

	luminance = clamp(max(black,luminance-luminosity_threshold)+max(black,luminance-luminosity_threshold)+max(black,luminance-luminosity_threshold),0.0,1.0);
	ao = lerp(ao,white,luminance);
	#endif

	color.rgb *= ao;

	#ifdef SHOW_SSAO
	return aoSample;
	#endif
	return color;
}

technique t0
{
	pass p0
	{
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader = compile ps_3_0 ssao_Main();
		ZEnable = false;
		SRGBWriteEnable = USE_SRGB;
	}
	pass p1
	{
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader = compile ps_3_0 HBlur();
		ZEnable = false;
		SRGBWriteEnable = USE_SRGB;
	}
	pass p2
	{
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader = compile ps_3_0 VBlur();
		ZEnable = false;
		SRGBWriteEnable = USE_SRGB;
	}
	pass p3
	{
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader = compile ps_3_0 Combine();
		ZEnable = false;
		SRGBWriteEnable = USE_SRGB;
	}	
}