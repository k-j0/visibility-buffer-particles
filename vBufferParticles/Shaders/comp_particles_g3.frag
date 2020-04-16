#version 450

/// Fragment shader for comp/comp particles in G-Buffer (3) renderer.

#define COMP_PARTICLE_FRAGMENT // <- set this flag as comp/comp pipelines bind textures to different locations
#include "particles_frag.glsl"

layout (location = 0) in vec2 iUv;

// G-Buffer writes
layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oPosition;
layout(location = 2) out vec4 oNormal;

void main(){
	
	oAlbedo = particleFragment(iUv);

	#ifdef PARTICLE_CUTOUT_MODE_1
	if(oAlbedo.a < 0.5) discard; // discard fragments based on transparency of texel
	#endif

	oPosition = oNormal = (0).xxxx; // no need for position or normal information for particle geometry

}// main
