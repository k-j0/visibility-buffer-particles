
#version 450

/// Vertex shader for comp/comp particles

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal; // unused - iNormal.x contains position.w in clip space.
layout(location = 2) in vec2 iUv;

layout (location = 0) out vec2 oUv;

void main(){
	
	gl_Position = vec4(iPosition, iNormal.x);
	oUv = iUv;

}// main
