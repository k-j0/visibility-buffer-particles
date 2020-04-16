#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Default vertex shader for static meshes

/// Uniform matrix buffer
layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 proj;
	float time;
} ubo;

/// Input per vertex; position, normal, uv
layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec2 iUv;

/// Output per vertex; uv, normal, position, time
layout(location = 0) out vec2 oUv;
layout(location = 1) out vec3 oWorldNormal;
layout(location = 2) out vec4 oWorldPosition;
layout(location = 3) out float oTime;


/// Transforms vertex position and normal from model space to clip space. Pass time and uvs through to next shader in pipeline.
void main(){
	oWorldPosition = ubo.model * vec4(iPosition, 1.0);
	oTime = ubo.time;
	oUv = iUv;
	oWorldNormal = normalize((ubo.model * vec4(iNormal, 0.0)).rgb);
	gl_Position = ubo.proj * ubo.view * oWorldPosition;
}
