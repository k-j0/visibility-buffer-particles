#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Fragment shader for static meshes with Raymarch material applied in Forward renderer

#include "raymarch.glsl"

layout(location = 0) in vec2 iUv;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in float iTime;

// framebuffer output
layout(location = 0) out vec4 oColor;

/// Data for one light
layout(binding = 2) uniform LightUBO{
	vec4 position;// xyz = position / w = radius
	vec4 diffuse;
	vec4 ambient;
} uboLight;
#include "lighting.glsl"


/// Returns lit version of the raymarch animation.
void main(){
    oColor = vec4(lightFragment(raymarch(iUv, iTime), worldPosition.xyz, worldNormal),1.0); // get albedo from common raymarch function, and light fragment immediately.
}