#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Fragment shader for static meshes in V-Buffer renderer first pass

layout(location = 0) in vec4 iVisibility;

// V-Buffer writes
layout(location = 0) out vec4 oVisibility;


/// Pass-through fragment shader to output visibility data per fragment to Visibility Buffer.
void main(){
	oVisibility = iVisibility;
	oVisibility.z += gl_PrimitiveID; // add the primitive ID within the current draw call (before this, the value is the offset of ids from other draw calls).
}