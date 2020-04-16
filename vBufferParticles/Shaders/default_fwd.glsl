
/// Default fragment shader for forward pipeline; must be included in a .frag after defining TEXTURE_BINDING to a valid uint.


/// Data for one light
layout(binding = 2) uniform LightUBO{
	vec4 position;// xyz = position / w = radius
	vec4 diffuse;
	vec4 ambient;
} uboLight;

#include "lighting.glsl"

// input per fragment
layout(location = 0) in vec2 iUv;
layout(location = 1) in vec3 iWorldNormal;
layout(location = 2) in vec4 iWorldPosition;
layout(location = 3) in float iTime;

// framebuffer write
layout(location = 0) out vec4 oColor;

// texture attachment
layout(binding = TEXTURE_BINDING) uniform sampler2D texSampler;



/// Sample texture, light fragment using the lighting.glsl include, and return resulting lit fragment.
void main(){
	
	vec3 albedo = texture(texSampler, iUv).rgb;
	
	oColor = vec4(lightFragment(albedo, iWorldPosition.xyz, iWorldNormal.xyz), 1.0);

	//oColor = vec4(1);
	
}
