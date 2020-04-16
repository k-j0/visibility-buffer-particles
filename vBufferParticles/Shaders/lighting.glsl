
/// Given a fragment's albedo, position and normal (in world space), and provided the following LightUBO definition, shades the fragment.
/// The reason LightUBO isn't defined explicitly here is for flexibility, as the function can be called from all rendering pipelines (forward, deferred, v-buffer).


// Before #include, LightUBO should be defined as:
/*

uniform LightUBO{
	vec4 position;// xyz = position / w = radius
	vec4 diffuse;
	vec4 ambient;
} uboLight;

*/

/// Given information about a fragment, returns its lit version
vec3 lightFragment(vec3 albedo, vec3 worldPos, vec3 worldNormal){
	
	// default colour is albedo * ambient colour.
	vec3 fragColor = albedo * uboLight.ambient.rgb;

	// lighting calculations
	{
		vec3 lightPos = uboLight.position.xyz;
		
		// frag to light
		vec3 toLight = lightPos - worldPos;
		float dist = length(toLight);
		toLight = normalize(toLight);
		
		// attenuation
		float atten = clamp(uboLight.position.w / (pow(dist, 2.0) + 1.0), 0.0, 1.0);
		
		// diffuse
		vec3 n = normalize(worldNormal);
		float nDotL = max(0.0, dot(n, toLight));
		vec3 diff = uboLight.diffuse.rgb * albedo.rgb * nDotL * atten;
		
		fragColor += diff;
		
	}

	return fragColor;

}
