
#include "particles_frag.glsl"

/// Shades particle fragments in v-buffer shading pass.
vec4 shadeParticleFragment(vec2 uv){
	
	return particleFragment(uv);

}

