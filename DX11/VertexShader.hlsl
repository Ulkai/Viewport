#include "ShaderDef.h"

VOut main(float4 position : POSITION, float2 uv : TEXCOORD0)
{
	VOut output;

	output.position = position;
	output.uv = uv;

	return output;
}