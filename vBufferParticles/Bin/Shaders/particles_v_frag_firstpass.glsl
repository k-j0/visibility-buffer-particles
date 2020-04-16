
/// First pass fragment shader for particles in v-buffer pipeline. In comp/comp mode, expects COMP_PARTICLE_FRAGMENT to be #defined prior to #include-ing this file.

#define PARTICLES_MAT 4 // must match other v-buffer shaders and VBufferScene.h

#include "../__.defines"

layout (location = 0) in vec2 iUv;

// V-Buffer writes
layout(location = 0) out vec4 oVisibility;

#ifdef PARTICLE_CUTOUT_MODE_1
#include "particles_frag.glsl"
#endif

void main(){
	
	#ifdef PARTICLE_CUTOUT_MODE_1
	// should this fragment be discarded based on texture
	vec4 tex = particleFragment(iUv);
	if(tex.a < 0.5) discard;
	#endif
	
	oVisibility = vec4(iUv, 0, PARTICLES_MAT);


}// main
