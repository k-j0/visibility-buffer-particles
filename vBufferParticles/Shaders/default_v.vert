#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Default vertex shader for static meshes in V-Buffer pipeline

/// Uniform matrix buffer
layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 proj;
	float time;
} ubo;

/// Input per vertex; position, uv, ids
layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUv;
layout(location = 2) in vec2 iIds; // x: triangle id, y: material id

/// Output per vertex; uv, triangle id, material id
layout(location = 0) out vec4 oVisibility;


/// Compute visibility data for this fragment, and transform position from world space to clip space. Pass visibility data out to fragment shader for write to V-Buffer.
void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(iPosition, 1.0);
	
	oVisibility = vec4(iUv, iIds);
	
}
