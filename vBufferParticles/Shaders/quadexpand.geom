#version 450

/// Geometry shader that expands vertices into quads; used in vert/geom particle generation mode.

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

/// Input per vertex; half size of the quad
layout(location = 0) in float[] iHalfSize;
layout (location = 1) in mat4[] iProjection;

/// Output per vertex; uv
layout(location = 0) out vec2 oUv;



const vec2 quadUVs[4] = {vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1)};

void main() {

	// Emit quad as 4-vertex triangle strip
	for(int j = 0; j < 4; ++j){
		vec2 uv = quadUVs[j];

		gl_Position = iProjection[0] * (gl_in[0].gl_Position + vec4(uv*iHalfSize[0], 0, 0));
		oUv = uv * 0.5 + 0.5; // 0..1
		EmitVertex();
	}
	EndPrimitive();

}
