
/// Default fragment shader for g-buffer pipeline; must be included in a .frag after defining TEXTURE_BINDING to a valid uint.



// near and far plane distances, for linear depth calculation
#define NEAR 0.01
#define FAR 100.0




layout(location = 0) in vec2 iUv;
layout(location = 1) in vec3 iWorldNormal;
layout(location = 2) in vec4 iWorldPosition;
layout(location = 3) in float iTime;

// G-Buffer writes
layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oPosition;
layout(location = 2) out vec4 oNormal;
layout(location = 3) out vec4 oEmission;
layout(location = 4) out vec4 oSpecular;
layout(location = 5) out vec4 oMetallicRoughness;

layout(binding = TEXTURE_BINDING) uniform sampler2D texSampler;





/// Makes depth linear.
float linearDepth(float depth){
	float z = (depth * 2.0 - 1.0) * (FAR - NEAR);
	return (2.0 * NEAR * FAR) / (FAR + NEAR - z);
}


/// Writes ws position & normal, and albedo (texture read) to G-Buffers
void main(){

	oPosition = vec4(iWorldPosition.xyz, 1.0);
	oPosition.w = linearDepth(gl_FragCoord.z);
	
	oNormal = vec4(normalize(iWorldNormal), 1.0);
	
	oAlbedo = texture(texSampler, iUv);
	oAlbedo.a = 1.0;

	oEmission = vec4(1, 1, 0, 1);
	oSpecular = vec4(1, 0, 1, 1);
	oMetallicRoughness = vec4(0.5);
	
}