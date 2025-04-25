// Cell shading from GSFx Shader Suite by Asmodean

extern float EdgeStrength = 0.70;                //[0.00 to 4.00] Overall strength of the cel edge outline effect. Affects the overall density.  0.00: no outlines.
extern float EdgeFilter = 0.60;                  //[0.10 to 2.00] Filters out fainter cel edges. Use it for balancing the cel edge density. EG: for faces, foliage, etc.
extern float EdgeThickness = 0.80;               //[0.50 to 4.00] Thickness of the cel edges. Increase for thicker outlining.  Note: when downsampling, raise this to keep the same thickness.
extern int PaletteType = 1;                      //[1|2|3] The color palette to use. 1 is Game Original, 2 is Animated Shading, 3 is Water Painting (Default is 2: Animated Shading).
extern int UseYuvLuma = 1;                       //[0 or 1] Uses YUV luma calculations, or base color luma calculations. Yuv luma can produce a better shaded look.
extern int ColorRounding = 1;                    //[0 or 1] Uses rounding methods on colors. This can emphasise shaded toon colors. Looks good in some games, and odd in others.

#ifndef PIXEL_SIZE // This is just for compilation-time syntax checking.
#define PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
#endif

static float2 rcpres = PIXEL_SIZE;

texture2D frameTex2D;
sampler frameSampler = sampler_state
{
    Texture = <frameTex2D>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MinFilter = POINT;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
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

static const float3 lumCoeff = float3(0.2126729, 0.7151522, 0.0721750);

//Average relative luminance
float AvgLuminance(float3 color)
{
    return sqrt(
    (color.x * color.x * lumCoeff.x) +
    (color.y * color.y * lumCoeff.y) +
    (color.z * color.z * lumCoeff.z));
}

float3 RGBtoYUV(float3 RGB)
{
    static const float3x3 m = {
    0.2126, 0.7152, 0.0722,
   -0.09991,-0.33609, 0.436,
    0.615, -0.55861, -0.05639 };

    return mul(m, RGB);
}

float3 YUVtoRGB(float3 YUV)
{
    static const float3x3 m = {
    1.000, 0.000, 1.28033,
    1.000,-0.21482,-0.38059,
    1.000, 2.12798, 0.000 };

    return mul(m, YUV);
}

float4 CelPass(VSOUT IN) : COLOR0
{
    float4 color = float4(tex2D(frameSampler, IN.UVCoord).rgb, 1.0);
    float3 yuv;
    float3 sum = color.rgb;

    const int NUM = 9;
    const float2 RoundingOffset = float2(0.25, 0.25);
    const float3 thresholds = float3(9.0, 8.0, 6.0);

    float lum[NUM];
    float3 col[NUM];
    float2 set[NUM] = {
    float2(-0.0078125, -0.0078125),
    float2(0.00, -0.0078125),
    float2(0.0078125, -0.0078125),
    float2(-0.0078125, 0.00),
    float2(0.00, 0.00),
    float2(0.0078125, 0.00),
    float2(-0.0078125, 0.0078125),
    float2(0.00, 0.0078125),
    float2(0.0078125, 0.0078125) };

    for (int i = 0; i < NUM; i++)
    {
        col[i] = tex2D(frameSampler, IN.UVCoord + set[i] * RoundingOffset).rgb;

        if (ColorRounding == 1)
        {
            col[i].r = round(col[i].r * thresholds.r) / thresholds.r;
            col[i].g = round(col[i].g * thresholds.g) / thresholds.g;
            col[i].b = round(col[i].b * thresholds.b) / thresholds.b;
        }

        lum[i] = AvgLuminance(col[i].xyz);
        yuv = RGBtoYUV(col[i]);

        if (UseYuvLuma == 0)
            yuv.r = round(yuv.r * thresholds.r) / thresholds.r;
        else
            yuv.r = saturate(round(yuv.r * lum[i]) / thresholds.r + lum[i]);
        
        yuv = YUVtoRGB(yuv);
        sum += yuv;
    }

    float3 shadedColor = (sum / NUM);
    float2 pixel = float2(rcpres.x * EdgeThickness, rcpres.y * EdgeThickness);

    float edgeX = dot(tex2D(frameSampler, IN.UVCoord + pixel).rgb, lumCoeff);
    edgeX = dot(float4(tex2D(frameSampler, IN.UVCoord - pixel).rgb, edgeX), float4(lumCoeff, -1.0));

    float edgeY = dot(tex2D(frameSampler, IN.UVCoord + float2(pixel.x, -pixel.y)).rgb, lumCoeff);
    edgeY = dot(float4(tex2D(frameSampler, IN.UVCoord + float2(-pixel.x, pixel.y)).rgb, edgeY), float4(lumCoeff, -1.0));

    float edge = dot(float2(edgeX, edgeY), float2(edgeX, edgeY));

    if (PaletteType == 1)
        color.rgb = lerp(color.rgb, color.rgb + pow(edge, EdgeFilter) * -EdgeStrength, EdgeStrength);

    if (PaletteType == 2)
        color.rgb = lerp(color.rgb + pow(edge, EdgeFilter) * -EdgeStrength, shadedColor, 0.25);

    if (PaletteType == 3)
        color.rgb = lerp(shadedColor + edge * -EdgeStrength, pow(edge, EdgeFilter) * -EdgeStrength + color.rgb, 0.5);

    color.a = AvgLuminance(color.rgb);

    return saturate(color);
}
 
technique t0
{
	pass p0
	{
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader = compile ps_3_0 CelPass();
		ZEnable = false;
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
		ColorWriteEnable = RED|GREEN|BLUE;
		SRGBWriteEnable = false;
	}
}