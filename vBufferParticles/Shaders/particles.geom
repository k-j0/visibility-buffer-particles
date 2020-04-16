#version 450

/// Geometry shader for geometry generation of particles in geom/geom mode; given empty input vertices, outputs meshes for up to 28 particles per invocation

#include "particles.glsl"

#define PARTICLES_PER_INPUT_VERTEX 28 // default: 28 (the maximum amount of quads that can be emitted from a single geometry shader call guaranteed by hardware conforming to the Vulkan spec.)

layout (points) in;
layout (triangle_strip, max_vertices = 4*PARTICLES_PER_INPUT_VERTEX) out;

/// Input per vertex; vertex index in the initial vertex buffer
layout(location = 0) in float iVertexIndex[];

/// Output per vertex; uv
layout(location = 0) out vec2 oUv;



const vec2 quadUVs[4] = {vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1)};

// from a vec3 representing the center, emit a quad with size 2*halfSize.
void quadify(vec3 centre, float halfSize){
	
	vec4 particleCenter = ubo.view * vec4(centre.xyz, 1);
	
	// Emit quad as 4-vertex triangle strip
	for(int j = 0; j < 4; ++j){
		vec2 uv = quadUVs[j];

		gl_Position = ubo.proj * (particleCenter + vec4(uv*halfSize, 0, 0)); // expand in view space before transformation to clip space.
		oUv = uv * 0.5 + 0.5; // 0..1
		EmitVertex();
	}
	EndPrimitive();

}// void quadify



void main() {

	vec4 pos;
	uint vId = uint(iVertexIndex[0]);// the index of the source vertex
	uint pId;

	// generate the required amount of particles
	for(uint p = 0; p < PARTICLES_PER_INPUT_VERTEX; ++p){
		pId = vId * PARTICLES_PER_INPUT_VERTEX + p;

		if(pId >= ubo.particleCount) return;// this is the last invocation, no need for any more particles
		
		pos = particle(pId);

		// ...Create a quad by expanding the position by the half size
		quadify(pos.xyz, pos.w);
		
	}

}
