#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Lighting pass fragment shader for V-Buffer renderer.


#include "../__.defines"


#define EXPECT_DEBUG_BUFFER // comment out to prevent receiving debug uniform data from CPU. See GBufferScene.h for CPU equivalent SEND_DEBUG_BUFFER


#define VBUFFER_MAX_TRIANGLES 64
#define VBUFFER_MAX_INDICES VBUFFER_MAX_TRIANGLES*3
#define VBUFFER_MAX_VERTICES VBUFFER_MAX_INDICES


#define SHRIMP_MAT 1
#define RAYMARCH_MAT 2
#define RACCOON_MAT 3
#define PARTICLES_MAT 4



#define CLEAR_COLOUR vec4(0.4, 0.4, 0.3, 1.0) // yellowish 'clear' colour for fragments with no triangles drawn.




#include "raymarch.glsl"



/// Data for one light
layout(binding = 1) uniform LightUBO{
	vec4 position;// xyz = position / w = radius
	vec4 diffuse;
	vec4 ambient;
} uboLight;
#include "lighting.glsl"


/// Uniform matrix buffer
layout(binding = 2) uniform MatrixUBO{
	mat4 model;
	mat4 view;
	mat4 proj;
	float time;
} uboMatrix;




/// Data for one stored vertex
struct VertexInput{
	vec4 position_u; // xyz: position; w: u
	vec4 normal_v; // xyz: normal; w: v
};

/// Index and Vertex buffers
layout(set = 0, binding = 3, std140) uniform VerticesUBO{
	uvec4 indices[VBUFFER_MAX_INDICES/4];// array of VBUFFER_MAX_INDICES uints, aligned to uvec4s. Access using uboVerticesGetIndex().
	VertexInput vertices[VBUFFER_MAX_VERTICES];
} uboVertices;

// use to access indices in index buffer for a range 0..VBUFFER_MAX_INDICES uints rather than 0..VBUFFER_MAX_INDICES/4 uvec4s.
uint uboVerticesGetIndex(uint i){
	return uboVertices.indices[i/4][i%4];
}

vec4 getVertexPos(VertexInput v){
	return vec4(v.position_u.xyz, 1.0);
}
vec4 getVertexNormal(VertexInput v){
	return vec4(v.normal_v.xyz, 0.0);
}
vec2 getVertexUV(VertexInput v){
	return vec2(v.position_u.w, v.normal_v.w);
}



#ifdef EXPECT_DEBUG_BUFFER
layout(binding = 7) uniform DebugUBO{
	float value;
} uboDebug;
#endif





/// Input data per fragment: screen uv coordinate
layout(location = 0) in vec2 iSPUv;

/// V-Buffer read from subpass input attachment
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iVisibility;

/// Shrimp texture
layout(binding = 4) uniform sampler2D shrimpSampler;
/// Raccoon texture
layout(binding = 5) uniform sampler2D raccoonSampler;
/// Particle texture
layout(binding = 6) uniform sampler2D particleSampler;


/// Output fragment colour
layout(location = 0) out vec4 oColor;









// multiply and add, should get compiled to MAD instruction (hopefully :))
float mad(float a, float b, float c){ return a*b + c; }
vec3 mad(vec3 a, vec3 b, vec3 c){ return a*b + c; }
vec4 mad(vec4 a, vec4 b, vec4 c){ return a*b + c; }
vec4 mad(vec4 a, float b, vec4 c){ return a*b + c; }

/// Linear interpolation between 3 vertices
VertexInput lerp3V(VertexInput vert0, VertexInput vert1, VertexInput vert2, vec3 h){
	VertexInput vIn;
	vIn.position_u = mad(vert0.position_u, h.x, mad(vert1.position_u, h.y, (vert2.position_u * h.z)));
	vIn.normal_v = mad(vert0.normal_v, h.x, mad(vert1.normal_v, h.y, (vert2.normal_v * h.z)));
	return vIn;
}

/// Converts a world-space vertex to clip space [-1..1]
vec4 world2clip(vec4 ws){
	return uboMatrix.proj * uboMatrix.view * ws;
}

/// Loads the vertex corresponding to the triangle index passed + the vertex index offset. Returns the vertex in world space
VertexInput loadVertex(uint triId, uint vId){
	
	// find the vertex in the buffer based on the index buffer at correct offset.
	uint index = uboVerticesGetIndex(triId * 3 + vId);
	VertexInput vert = uboVertices.vertices[index];

	vert.position_u = vec4((uboMatrix.model * getVertexPos(vert)).xyz, vert.position_u.w);//set world space position

	return vert;

}

/// Returns the area of a 2D triangle
float area2(vec2 a, vec2 b, vec2 c){
	return abs(a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) * 0.5;
}

/// Returns normalized homogeneous barycentric coordinates for a point p in a 2D triangle a b c
vec3 getBarycentricCoord(vec2 p, vec2 a, vec2 b, vec2 c){
	
	//find area of the 3 sub triangles abp, acp, bcp
	float areaA = area2(b, c, p);
	float areaB = area2(a, c, p);
	float areaC = area2(a, b, p);

	// normalize
	float total = areaA + areaB + areaC;

	return vec3(areaA / total, areaB / total, areaC / total);

}

/// Loads vertices based on triangle ID for the current fragment; returns an interpolation of the 3 vertices in the triangle at the current position
VertexInput loadAndLerpVerticesInTriangle(uint triId, vec2 uvs){
	
	VertexInput v0 = loadVertex(triId, 0);
	VertexInput v1 = loadVertex(triId, 1);
	VertexInput v2 = loadVertex(triId, 2);

	vec3 barycentric = getBarycentricCoord(uvs, getVertexUV(v0), getVertexUV(v1), getVertexUV(v2));

	return lerp3V(v0, v1, v2, barycentric);
}




/// Shade a fragment by applying a texture and lighting.
vec3 shadeTextured(VertexInput v, sampler2D tex){
	vec3 worldPos = getVertexPos(v).xyz;
	vec3 worldNormal = getVertexNormal(v).xyz;
	vec2 uv = getVertexUV(v);

	
	vec3 albedo = texture(tex, uv).rgb;

	return lightFragment(albedo, worldPos, worldNormal);
}


// Shrimp fragment shader
vec3 shadeFragmentShrimp(VertexInput v){
	return shadeTextured(v, shrimpSampler);
}

// Raccoon fragment shader
vec3 shadeFragmentRaccoon(VertexInput v){
	return shadeTextured(v, raccoonSampler);
}



/// Depending on the particle mode, use different textures
#define VISIBILITY_BUFFER_PARTICLE_FRAGMENT
#ifdef PARTICLE_CUTOUT_MODE_1
#define texSampler particleSampler
#else
#define texSampler shrimpSampler
#endif
#include "particles_v.glsl" // provides the definition for shadeParticleFragment().
#undef texSampler




/// Main function - loads data from visibility buffer, and outputs the corresponding fragment colour (albedo + lighting)
void main(){
	
	// Read V-Buffer and extract uv, triangle ID, material ID
	vec4 visibility = subpassLoad(iVisibility);
	vec2 uv = visibility.xy;
	float triIdF = visibility.z;
	float matIdF = visibility.w;
	uint triId = uint(triIdF+0.5);
	uint matId = uint(matIdF+0.5);


	// Caution: potential wavefront divergence here :)

	
	// A material ID of 0 means no objects on the current fragment.
	if(matId == 0){
		oColor = CLEAR_COLOUR;
	}else if(matId == PARTICLES_MAT){
		
		// Particle fragment here
		oColor = shadeParticleFragment(uv);

	}else{
		
		// load vertex from barycentric coordinates and transform to world space
		VertexInput v = loadAndLerpVerticesInTriangle(triId, uv);

		//execute "fragment" shader code for vertex to get final fragment color
		if(matId == SHRIMP_MAT){// Shrimp
			oColor = vec4(shadeFragmentShrimp(v), 1.0);
		}else if(matId == RAYMARCH_MAT){// Raymarch
			oColor = vec4(lightFragment(raymarch(getVertexUV(v), uboMatrix.time), getVertexPos(v).xyz, getVertexNormal(v).xyz), 1.0);
		}else if(matId == RACCOON_MAT){// Raccoon
			oColor = vec4(shadeFragmentRaccoon(v), 1.0);
		}else{// Undefined material
			oColor = vec4(1.0, 0.0, 1.0, 0.0);// magenta.
		}
	}


#ifdef EXPECT_DEBUG_BUFFER
	// Show debug views
	if(uboDebug.value == 1){// Visibility UV
		oColor = vec4(uv, 0, 1);
	}else if(uboDebug.value == 2){// Primitive ID
		oColor = vec4(triIdF / VBUFFER_MAX_TRIANGLES);
	}else if(uboDebug.value == 3){// Material ID
		oColor = vec4(matIdF / 4);
	}
#endif

	
}
