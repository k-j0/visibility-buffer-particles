#version 450

/// Common fragment shader for particles in G-Buffer (3) renderer, except comp/comp particles.

#include "particles_frag.glsl"

layout (location = 0) in vec2 iUv;

// G-Buffer writes
layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oPosition;
layout(location = 2) out vec4 oNormal;

void main(){
	
	oAlbedo = particleFragment(iUv);

	#ifdef PARTICLE_CUTOUT_MODE_1
	if(oAlbedo.a < 0.5) discard; // <- selectively discard fragments based on texel transparency
	#endif

	oPosition = oNormal = (0).xxxx; // <- no need for this data for particles

}// main
