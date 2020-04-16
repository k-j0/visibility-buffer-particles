#version 450

/// Common fragment shader for particles in forward pipeline (except comp/comp particles).

#include "particles_frag.glsl"

layout (location = 0) in vec2 iUv;

layout (location = 0) out vec4 oColour;

void main(){
	
	oColour = particleFragment(iUv);

	#ifdef PARTICLE_CUTOUT_MODE_1
	if(oColour.a < 0.5) discard; // <- discard fragments based on texel opacity
	#endif

}// main
