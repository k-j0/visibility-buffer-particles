
/// Provides the definition for particle() function which, given a particle index, returns its position and half-size at time t.


#include "random.glsl"

layout (set = 0, binding = 0) uniform UBO {
	mat4 view;
	mat4 proj;
	float time;
	float halfSize;
	float density;
	float gravity;
	float initialUpwardsForce;
	uint particleCount;
} ubo;

#define VERTICES_PER_PARTICLE 6


/// Returns the center of a particle based on the particle's index in view space
/// The half-size of the particle is returned as the w coordinate
vec4 particle(uint particleIndex){
	vec3 position;
	float size;

	vec3 origin = (0).xxx;

	vec3 seed = float(particleIndex).xxx + vec3(0, 1928.219, 2109.3002);
	vec3 rand = vec3(random(seed.x), random(seed.y), random(seed.z));// 0..1

	

	vec3 direction = normalize(rand-0.5)*2;// random point on sphere of radius 1 and center 0
	direction *= random(rand.x) * ubo.density; // map length of direction to 0..density
	float lifetime = mod(ubo.time+random(seed.x+seed.y), 1); // from 0 to 1 over the particle's lifetime

	position = origin + (direction+vec3(0, ubo.initialUpwardsForce, 0)) * lifetime + vec3(0, -ubo.gravity, 0) * lifetime * lifetime;
	size = 1-abs(0.5-lifetime)*2;// 0 -> 1 -> 0
	size *= ubo.halfSize;

	return vec4(position, size);
}
