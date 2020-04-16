#version 450

/// Fragment shader for comp/comp particles in Forward renderer.

#define COMP_PARTICLE_FRAGMENT // <- set this flag as comp/comp pipelines bind textures to different locations
#include "particles_frag.glsl"

layout (location = 0) in vec2 iUv;

layout (location = 0) out vec4 oColour;

void main(){
	
	oColour = particleFragment(iUv);

	#ifdef PARTICLE_CUTOUT_MODE_1
	if(oColour.a < 0.5) discard; // discard fragments based on transparency of texel
	#endif

}// main
