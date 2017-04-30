
SamplerState s0;
Texture2D tex0;

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
	return tex0.Sample(s0, uv);
}