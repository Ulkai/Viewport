#include "ShaderDef.h"

VOut main(float4 position : POSITION, float4 color : COLOR)
{
	VOut output;

	output.position = position;
	output.color = color;

	return output;
}