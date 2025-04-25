// HDR Scene Tonemapping from GSFx Shader Suite by Asmodean

extern int TonemapType = 1;              //[0|1|2|3] The base tone mapping operator. 0 is LDR, 1 is HDR(original), 2 & 3 are Filmic HDR(slight grading).
extern int TonemapMask = 0;              //[0 or 1] Enables an ALU tone masking curve. Produces a nice cinematic look. Suits some games more than others.
extern float MaskStrength = 0.30;        //[0.000 to 1.000] Strength of the tone masking. Higher for a stronger effect. This is a dependency of TonemapMask.
extern float ToneAmount = 0.300;         //[0.050 to 1.000] Tonemap strength (tone correction). Higher for stronger tone mapping, lower for lighter.
extern float BlackLevels = 0.060;        //[0.000 to 1.000] Black level balance (shadow correction). Increase to deepen blacks, lower to lighten them.
extern float Exposure = 1.000;           //[0.100 to 2.000] White correction (brightness). Higher values for more scene exposure, lower for less.
extern float Luminance = 1.000;          //[0.100 to 2.000] Luminance average (luminance correction). Higher values will lower scene luminance average.
extern float WhitePoint = 0.940;         //[0.100 to 2.000] Whitepoint average (wp lum correction). Higher values will lower the maximum scene white point.

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

static float Epsilon = 1e-10;
static const float3 lumCoeff = float3(0.2126729, 0.7151522, 0.0721750);

//Average relative luminance
float AvgLuminance(float3 color)
{
    return sqrt(
    (color.x * color.x * lumCoeff.x) +
    (color.y * color.y * lumCoeff.y) +
    (color.z * color.z * lumCoeff.z));
}

float3 EncodeGamma(float3 color, float gamma)
{
    color = saturate(color);
    color.r = (color.r <= 0.0404482362771082) ?
    color.r / 12.92 : pow((color.r + 0.055) / 1.055, gamma);
    color.g = (color.g <= 0.0404482362771082) ?
    color.g / 12.92 : pow((color.g + 0.055) / 1.055, gamma);
    color.b = (color.b <= 0.0404482362771082) ?
    color.b / 12.92 : pow((color.b + 0.055) / 1.055, gamma);

    return color;
}

float3 ScaleLuminance(float3 x)
{
    const float W = 1.02;
    const float L = 0.06;
    const float C = 1.02;

    const float N = clamp(0.76 + ToneAmount, 1.0, 2.0);
    const float K = (N - L * C) / C;

    float3 tone = L * C + (1.0 - L * C) * (1.0 + K * (x - L) /
    ((W - L) * (W - L))) * (x - L) / (x - L + K);

    return (x > L) ? tone : C * x;
}

float3 TmMask(float3 color)
{
    float3 tone = color;

    float highTone = 6.2; float greyTone = 0.5;
    float midTone = 1.620; float lowTone = 0.066;

    tone.r = (tone.r * (highTone * tone.r + greyTone))/(
    tone.r * (highTone * tone.r + midTone) + lowTone);
    tone.g = (tone.g * (highTone * tone.g + greyTone))/(
    tone.g * (highTone * tone.g + midTone) + lowTone);
    tone.b = (tone.b * (highTone * tone.b + greyTone))/(
    tone.b * (highTone * tone.b + midTone) + lowTone);

    static const float gamma = 2.42;
    tone = EncodeGamma(tone, gamma);

    color = lerp(color, tone, float(MaskStrength));

    return color;
}

float3 TmCurve(float3 color)
{
    float3 T = color;

    float tnamn = ToneAmount;
    float blevel = length(T);
    float bmask = pow(blevel, 0.02);

    const float A = 0.100; const float B = 0.300;
    const float C = 0.100; const float D = tnamn;
    const float E = 0.020; const float F = 0.300;

    const float W = 1.000;

    T.r = ((T.r*(A*T.r + C*B) + D*E) / (T.r*(A*T.r + B) + D*F)) - E / F;
    T.g = ((T.g*(A*T.g + C*B) + D*E) / (T.g*(A*T.g + B) + D*F)) - E / F;
    T.b = ((T.b*(A*T.b + C*B) + D*E) / (T.b*(A*T.b + B) + D*F)) - E / F;

    float denom = ((W*(A*W + C*B) + D*E) / (W*(A*W + B) + D*F)) - E / F;

    float3 black = float3(bmask, bmask, bmask);
    float3 white = float3(denom, denom, denom);

    T = T / white;
    T = T * black;

    color = saturate(T);

    return color;
}

float4 TonemapPass(VSOUT IN) : COLOR0
{
    float3 color = tex2D(frameSampler, IN.UVCoord).rgb;
    float3 tonemap = color;

    float blackLevel = length(tonemap);
    tonemap = ScaleLuminance(tonemap);

    float luminanceAverage = AvgLuminance(Luminance);

    if (TonemapMask == 1) { tonemap = TmMask(tonemap); }
    if (TonemapType == 1) { tonemap = TmCurve(tonemap); }

    // RGB -> XYZ conversion
    static const float3x3 RGB2XYZ = { 0.4124564, 0.3575761, 0.1804375,
                                      0.2126729, 0.7151522, 0.0721750,
                                      0.0193339, 0.1191920, 0.9503041 };

    float3 XYZ = mul(RGB2XYZ, tonemap);

    // XYZ -> Yxy conversion
    float3 Yxy;

    Yxy.r = XYZ.g;                                  // copy luminance Y
    Yxy.g = XYZ.r / (XYZ.r + XYZ.g + XYZ.b);        // x = X / (X + Y + Z)
    Yxy.b = XYZ.g / (XYZ.r + XYZ.g + XYZ.b);        // y = Y / (X + Y + Z)

    // (Wt) Tone mapped scaling of the initial wp before input modifiers
    float Wt = saturate(Yxy.r / AvgLuminance(XYZ));

    if (TonemapType == 2) { Yxy.r = TmCurve(Yxy).r; }

    // (Lp) Map average luminance to the middlegrey zone by scaling pixel luminance
    float Lp = Yxy.r * float(Exposure) / (luminanceAverage + Epsilon);

    // (Wp) White point calculated, based on the toned white, and input modifier
    float Wp = dot(abs(Wt), float(WhitePoint));

    // (Ld) Scale all luminance within a displayable range of 0 to 1
    Yxy.r = (Lp * (1.0 + Lp / (Wp * Wp))) / (1.0 + Lp);

    // Yxy -> XYZ conversion
    XYZ.r = Yxy.r * Yxy.g / Yxy.b;                  // X = Y * x / y
    XYZ.g = Yxy.r;                                  // copy luminance Y
    XYZ.b = Yxy.r * (1.0 - Yxy.g - Yxy.b) / Yxy.b;  // Z = Y * (1-x-y) / y

    if (TonemapType == 3) { XYZ = TmCurve(XYZ); }

    // XYZ -> RGB conversion
    static const float3x3 XYZ2RGB = { 3.2404542,-1.5371385,-0.4985314,
                                     -0.9692660, 1.8760108, 0.0415560,
                                      0.0556434,-0.2040259, 1.0572252 };

    tonemap = mul(XYZ2RGB, XYZ);

    float shadowmask = pow(saturate(blackLevel), float(BlackLevels));
    tonemap = tonemap * float3(shadowmask, shadowmask, shadowmask);

    float alpha = AvgLuminance(color.rgb);

    return float4(tonemap, alpha);
}

technique t0
{
	pass p0
	{
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader = compile ps_3_0 TonemapPass();
		ZEnable = false;
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
		ColorWriteEnable = RED|GREEN|BLUE;
		SRGBWriteEnable = false;
	}	
}
