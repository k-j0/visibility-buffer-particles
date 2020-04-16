#version 450

/// Common fragment shader for particles in G-Buffer (6) renderer, except comp/comp particles

#include "particles_frag.glsl"

layout (location = 0) in vec2 iUv;

// G-Buffer writes
layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oPosition;
layout(location = 2) out vec4 oNormal;
layout(location = 3) out vec4 oEmission;
layout(location = 4) out vec4 oSpecular;
layout(location = 5) out vec4 oMetallicRoughness;

void main(){
	
	oAlbedo = oEmission = particleFragment(iUv);

	#ifdef PARTICLE_CUTOUT_MODE_1
	if(oAlbedo.a < 0.5) discard; // <- discard fragments based on texel transparency
	#endif

	oPosition = oNormal = oSpecular = oMetallicRoughness = (0).xxxx; // <- no need for this data for particles.

}// main
