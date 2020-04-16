#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Fragment shader for static meshes with Raymarch material applied in G-Buffer (6) renderer

#include "raymarch.glsl"

// near and far plane distances, for linear depth calculation
#define NEAR 0.01
#define FAR 100.0

layout(location = 0) in vec2 iUv;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in float iTime;

// G-Buffer writes
layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oPosition;
layout(location = 2) out vec4 oNormal;
layout(location = 3) out vec4 oEmission;
layout(location = 4) out vec4 oSpecular;
layout(location = 5) out vec4 oMetallicRoughness;


float linearDepth(float depth){
	float z = (depth * 2.0 - 1.0) * (FAR - NEAR);
	return (2.0 * NEAR * FAR) / (FAR + NEAR - z);
}




/// fills G-Buffer with raymarch animation albedo + ws position and normal.
void main(){

    oAlbedo = vec4(raymarch(iUv, iTime),1.0); // get albedo from common raymarch function
	
	
	oPosition = vec4(worldPosition.xyz, 1.0);
	oPosition.w = linearDepth(gl_FragCoord.z);
	
	oNormal = vec4(normalize(worldNormal), 1.0);

	oEmission = vec4(1, 1, 1, 1);
	oSpecular = vec4(0, 0.5f, 0, 0);
	oMetallicRoughness = vec4(0.1f, 0.9f, 0, 0);

}