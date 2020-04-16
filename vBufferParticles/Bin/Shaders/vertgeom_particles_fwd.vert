
#version 450

/// vert/geom particles vertex shader

#include "particles.glsl"

layout (location = 0) out float oHalfSize;
layout (location = 1) out mat4 oProjection;

void main(){

	vec4 p = particle(gl_VertexIndex);
	oHalfSize = p.w;
	gl_Position = ubo.view * vec4(p.xyz, 1);
	oProjection = ubo.proj;

}// main
