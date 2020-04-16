
#version 450

/// vert/vert particles vertex shader

#include "particles.glsl"

layout (location = 0) out vec2 oUv;

// static UV multipliers for the 6 vertices of a quad
const vec2 staticUVs[6] = {vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, -1), vec2(1, 1), vec2(-1, 1)};

void main(){
	
	// find particle index and vertex index within the particle
	uint index = gl_VertexIndex;
	uint pIndex = index / 6;
	uint vIndex = index % 6;

	vec2 uv = staticUVs[vIndex];
	
	// generate the particle's position and size
	vec4 p = particle(pIndex);
	vec4 particleCenter = ubo.proj * ((ubo.view * vec4(p.xyz, 1)) + vec4(uv * p.w, 0, 0)); // expand to quad in view space before projecting to clip space.

	// fill output data
	gl_Position = particleCenter;
	oUv = uv * 0.5 + 0.5;

}// main
