

/// Provides definition for particleFragment() function which, given a particle's UVs, returns the final colour; output will heavily depend on contents of __.defines file, modified at runtime each time this shader is recompiled.


#include "../__.defines"




#ifndef VISIBILITY_BUFFER_PARTICLE_FRAGMENT // in V-Buffer, texture is provided by lighting pass' fragment shader instead.
	#ifdef PARTICLE_COMPLEXITY_2
		// determine where the texture is bound (0 if the particles were created via compute only, 1 otherwise)
		#ifdef COMP_PARTICLE_FRAGMENT
			#define TEX_BINDING 0
		#else
			#define TEX_BINDING 1
		#endif
		// texture attachment
		layout(binding = TEX_BINDING) uniform sampler2D texSampler;
	#endif
#endif

#ifdef PARTICLE_COMPLEXITY_3
	#include "raymarch.glsl"
#endif




/// Shades a particle fragment from its UV coordinate.
vec4 particleFragment(vec2 uv){
	
#ifdef PARTICLE_COMPLEXITY_0
	return vec4(uv, 1, 1); // simple passing of argument
#elif defined(PARTICLE_COMPLEXITY_1)
	return vec4(uv.x < 0.9 && uv.y < 0.9 ? sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(uv)))))))))) : uv.xy, 0, 1); // lots of sqrt's, and trivial wavefront divergence
#elif defined(PARTICLE_COMPLEXITY_2)
	return texture(texSampler, 1-uv); // texture sampling
#elif defined(PARTICLE_COMPLEXITY_3)
	return vec4(raymarch(1-uv, 0), 1); // lots of maths, and non-trivial wavefront divergence
#else
	return (1).xxxx; // white - unimplemented complexity level
#endif

}

