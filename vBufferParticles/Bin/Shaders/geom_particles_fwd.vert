
#version 450

/// (almost passthrough) Vertex shader for geom/geom particles.

layout (location = 0) out float oVertexIndex;

void main(){
	
	oVertexIndex = gl_VertexIndex;

}// main
