#include "ShaderDef.h"

VOut main(float4 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD0)
{
	VOut output;

	output.position = position;
	output.color = color;
	output.uv = uv;

	return output;
}