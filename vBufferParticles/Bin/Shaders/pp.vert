#version 450

/// Vertex shader for fullscreen quad, to be used for post-processing/lighting passes.

layout (location = 0) out vec2 oUv;

// Creates fullscreen quad for post-processing rendering; pass 0..1 screen UVs to fragment shader.
void main(){
	// Implementation inspired by Sascha Willems
	// https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/deferred/deferred.vert
	oUv = vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2);
	gl_Position = vec4(oUv * 2.0 - 1.0, 0.0, 1.0);
}