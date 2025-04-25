// Depth Of Field Effect by WrinklyNinja. Release Version 7.
// Modified for Ephinea Phantasy Star Online: Blue Burst by Sodaboy

// ---------------------------------------
// TWEAKABLE VARIABLES.
 
extern float nearZ = 0.01;
extern float farZ = 1.0;

extern float2 FocusPosition = float2 (0.5,0.9);

extern float FullFocusRange = 0.1;
//If a point's depth is within +/- this value of the depth of the focus point, then it is not blurred.
 
extern float NoFocusRange = 0.4;
//If a point's depth is outwith +/- this value of the depth of the focus point, then it has the maximum level of blur applied to it.

extern float DoFAmount = 25;
//The maximum level of blur. A higher value will give you more blur.
  
#define	DISTANTBLUR
//If defined, reduces DoF to a distance blurring effect, ie. things far away are blurred more than things close up.
 
// END OF TWEAKABLE VARIABLES.
// ---------------------------------------

#ifndef PIXEL_SIZE // This is just for compilation-time syntax checking.
#define PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
#endif

static float2 rcpres = PIXEL_SIZE;

texture2D frameTex2D;
sampler2D frameSampler = sampler_state
{
    Texture = <frameTex2D>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MinFilter = POINT;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
};


texture2D depthTex2D;
sampler2D depthSampler = sampler_state
{
    Texture = <depthTex2D>;
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
 
float ComputePointBlurMagnitude (float focusdepth, float pointdepth)
{
	float difference = abs(pointdepth - focusdepth);
	float blurmag;
 
	if (difference <= FullFocusRange)
		blurmag = 0;
	else if (difference >= NoFocusRange)
		blurmag = 1;
	else
		blurmag = difference / (NoFocusRange - FullFocusRange);
 
	if (pointdepth < 0.6) // Don't blur if too close...
		blurmag = 0;
 
#ifdef	DISTANTBLUR
	if (pointdepth == 1)
		blurmag = 0;
#endif

    if (blurmag > 1.0) // Clip (Not sure this is needed!)
        blurmag = 1.0;
 
	return blurmag * DoFAmount;
}
 
float4 ComputeBlurVector (float FocusDepth, float sample, float4 SurroundingPoints) {
	// Rule for blurring is: Points may only blur onto points which are behind them.
	float4 blurdirection = float4(0,0,0,0); //Unit blur vector. Directions in order are: (x+,X-,y+,y-).
 
	// Point is in foreground.
	if (sample < FocusDepth)
		blurdirection = float4(1,1,1,1); //Point can blur in any direction.
 
	// Point is in background.
	else if (sample > FocusDepth) {
		if (sample <= SurroundingPoints.x) //Point is front of point one to the right.
			blurdirection.x = 1;
		if (sample <= SurroundingPoints.y) //Point is front of point one to the left.
			blurdirection.y = 1;
		if (sample <= SurroundingPoints.z) //Point is front of point one up.
			blurdirection.z = 1;
		if (sample <= SurroundingPoints.w) //Point is front of point one down.
			blurdirection.w = 1;
	}
	// In all other cases, do not blur.
 
	float blurmagnitude = ComputePointBlurMagnitude(FocusDepth, sample);
 
	return blurdirection * blurmagnitude;
}
 
// Does a 2D Gaussian blur, in the specified directions by the specified amount.
float4 DoGaussianBlur(float2 UVCoord, float4 blurvector) {
	static const float BlurWeights[13] = {
		0.057424882f,
		0.058107773f,
		0.061460144f,
		0.071020611f,
		0.088092873f,
		0.106530916f,
		0.114725602f,
		0.106530916f,
		0.088092873f,
		0.071020611f,
		0.061460144f,
		0.058107773f,
		0.057424882f
	};
 
	static const float2 BlurOffsets[13] = {
		float2(-6.0f * rcpres.x, -6.0f * rcpres.y),
		float2(-5.0f * rcpres.x, -5.0f * rcpres.y),
		float2(-4.0f * rcpres.x, -4.0f * rcpres.y),
		float2(-3.0f * rcpres.x, -3.0f * rcpres.y),
		float2(-2.0f * rcpres.x, -2.0f * rcpres.y),
		float2(-1.0f * rcpres.x, -1.0f * rcpres.y),
		float2( 0.0f * rcpres.x,  0.0f * rcpres.y),
		float2( 1.0f * rcpres.x,  1.0f * rcpres.y),
		float2( 2.0f * rcpres.x,  2.0f * rcpres.y),
		float2( 3.0f * rcpres.x,  3.0f * rcpres.y),
		float2( 4.0f * rcpres.x,  4.0f * rcpres.y),
		float2( 5.0f * rcpres.x,  5.0f * rcpres.y),
		float2( 6.0f * rcpres.x,  6.0f * rcpres.y)
	};
 
	float4 Color = 0;
 
	for (int i = 0; i < 13; i++) {
		//Y axis blur.
		if (i < 6) //Negative y blur.
			Color += tex2D(frameSampler, UVCoord + (BlurOffsets[i] * float2(0,1) * blurvector.w) * BlurWeights[i]);
		else if (i ==6)
			Color += tex2D(frameSampler, UVCoord);
		else //Positive y blur.
			Color += tex2D(frameSampler, UVCoord + (BlurOffsets[i] * float2(0,1) * blurvector.z) * BlurWeights[i]);
 
		//X axis blur.
		if (i < 6) //Negative x blur.
			Color += tex2D(frameSampler, UVCoord + (BlurOffsets[i] * float2(1,0) * blurvector.y) * BlurWeights[i]);
		else if (i == 6)
			Color += tex2D(frameSampler, UVCoord);
		else //Positive x blur.
			Color += tex2D(frameSampler, UVCoord + (BlurOffsets[i] * float2(1,0) * blurvector.x) * BlurWeights[i]);
	}
 
	return Color / 26;
}
 
float LinearDepth(in float2 coord : TEXCOORD0)
{
	float z = tex2D(depthSampler, coord).r;
	return (2.0f * nearZ) / (nearZ + farZ - z * (farZ - nearZ));
}

//Main DoF effect function.
float4 DoFEffect(VSOUT IN) : COLOR0
{
	//Get depths for sampled point, focus point and the points surrounding the sampled point.
	float sample = LinearDepth(IN.UVCoord); //Get depth of sampled point.
	float FocusDepth;
 
	FocusDepth = LinearDepth(FocusPosition); //Focus is at specified point.
 
	float4 SurroundingPoints = float4(1,1,1,1);
 
	SurroundingPoints.x = LinearDepth(IN.UVCoord + float2(rcpres.x, rcpres.y) * float2(-1, 0)); //Right one pixel
	SurroundingPoints.y = LinearDepth(IN.UVCoord + float2(rcpres.x, rcpres.y) * float2( 1, 0)); //Left one pixel
	SurroundingPoints.z = LinearDepth(IN.UVCoord + float2(rcpres.x, rcpres.y) * float2( 0,-1)); //Up one pixel
	SurroundingPoints.w = LinearDepth(IN.UVCoord + float2(rcpres.x, rcpres.y) * float2( 0, 1)); //Down one pixel
 
	//Calculate blur vector (directions + magnitude) and apply blur.
	float4 BlurVector = ComputeBlurVector(FocusDepth, sample, SurroundingPoints);
	return float4(DoGaussianBlur(IN.UVCoord, BlurVector).rgb, 1);
}
 
technique t0
{
	pass p0 {
		VertexShader = compile vs_3_0 FrameVS();
		PixelShader  = compile ps_3_0 DoFEffect();
		ZEnable = false;
	}
}