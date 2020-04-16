#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Lighting pass fragment shader for G-Buffer (3) renderer

#define EXPECT_DEBUG_BUFFER // comment out to prevent receiving debug uniform data from CPU. See GBufferScene.h for CPU equivalent SEND_DEBUG_BUFFER




#define CLEAR_COLOUR vec4(0.3, 0.3, 0.4, 1.0) // Blue "clear" colour



// near and far plane distances, for linear depth calculation
#define NEAR 0.01
#define FAR 100.0


/// Data for one light
layout(binding = 3) uniform LightUBO{
	vec4 position;// xyz = position / w = radius
	vec4 diffuse;
	vec4 ambient;
} uboLight;
#include "lighting.glsl"


/// Input data per fragment: screen uv coordinate
layout(location = 0) in vec2 iUv;

/// G-Buffer reads (albedo, position, normal)
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iAlbedo;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput iPosition;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput iNormal;


/// Output fragment colour
layout(location = 0) out vec4 oColor;

#ifdef EXPECT_DEBUG_BUFFER
layout(binding = 4) uniform DebugUBO{
	float value;
} uboDebug;
#endif




/// Load G-Buffer data (albedo, ws position & ws normal) from subpass attachments and compute fragment colour from light info.
void main(){
	
	vec4 albedo = subpassLoad(iAlbedo);
	vec4 position_depth = subpassLoad(iPosition);
	vec3 normal = subpassLoad(iNormal).rgb;

	vec3 position = position_depth.rgb;
	
	// Light fragment
	float litAmount = clamp(length(normal*999), 0, 1);// 0 where normal == (0,0,0), 1 otherwise.
	oColor = mix(vec4(albedo.rgb, 1), vec4(lightFragment(albedo.rgb, position, normal), 1.0), litAmount);// fragments where normal == (0,0,0) receive no light, other fragments receive full light.

	
	// Blue "clear" colour for areas where alpha == 0 in albedo texture
	oColor = mix(oColor, CLEAR_COLOUR, 1.0-albedo.w);

#ifdef EXPECT_DEBUG_BUFFER
	// Show debug views
	if(uboDebug.value == 1){// Depth
		oColor = mix(position_depth.wwww / FAR, vec4(1.0, 1.0, 1.0, 1.0), 1.0-albedo.w);
	}else if(uboDebug.value == 2){// Albedo
		oColor = albedo;
	}else if(uboDebug.value == 3){// WS Position
		oColor = vec4(position, 1);
	}else if(uboDebug.value == 4){// WS Normal
		oColor = vec4(normal, 1);
	}
#endif

}
