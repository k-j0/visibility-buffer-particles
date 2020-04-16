
/// Raymarch fragment shader. Adapted from my own shadertoy: https://www.shadertoy.com/view/ts3GRf

#ifndef RAYMARCH_I // include guard
#define RAYMARCH_I

#define MAX_STEPS 1000
#define MAX_DISTANCE 100.
#define MIN_DISTANCE 0.02


//https://www.iquilezles.org/www/articles/smin/smin.htm
float smin( float a, float b, float k ){
    float res = exp2( -k*a ) + exp2( -k*b );
    return -log2( res )/k;
}

// distance to sphere function
float sdSphere(vec3 p, vec4 s){ return length(s.xyz-p) - s.w; }

// distance function for scene
float Distance(vec3 p, float time){
    vec4 s1 = vec4(0., 1.+sin(time*0.564)*0.1, 0., 1.);
    vec4 s2 = vec4(2.*sin(time), 1., 0., 0.4);
    vec4 s3 = vec4(0., 1., 0.2, 1.);
    
    float sd = sdSphere(p, s1);
    sd = max(-sdSphere(p, s3), sd);
    sd = smin(sd, sdSphere(p, s2), 5.);
    float pd = abs(p.y);
    
    float d = smin(sd, pd, 5.);
    return d;
}

// cast a single ray in a certain direction and return distance
float CastRay(vec3 o, vec3 dir, float time){
    float d = 0.;
    for(int i = 0; i<MAX_STEPS; ++i){
        float dist = Distance(o+dir*d, time);
        d += dist;
        if(dist < MIN_DISTANCE || d > MAX_DISTANCE) break;
    }
    return d;
}

// get normal at a certain point in 3D space, using small step size to sample the distance field
vec3 GetNormal(vec3 p, float time){
	float d = Distance(p, time);
    vec2 e = vec2(.01, 0.);
    vec3 n = d - vec3(Distance(p-e.xyy, time), Distance(p-e.yxy, time), Distance(p-e.yyx, time));
    return normalize(n);
}

// light a fragment using the world position and light position
float Light(vec3 p, vec3 lightPos, float time){
    vec3 toLight = normalize(lightPos - p);
    vec3 n = GetNormal(p, time);
    
   	float diffuse = dot(n, toLight);
    
    float distToLight = CastRay(p + n * MIN_DISTANCE * 2., n, time) / length(lightPos-p);
    diffuse *= clamp(distToLight, 0., 1.);
    
    return clamp(diffuse, 0., 1.);
}



// returns raymarch animation given 0..1 UVs and current time.
vec3 raymarch(vec2 UV, float time){
	vec2 uv = (UV - 0.5) * 2.;
	uv.y = -uv.y;

    vec3 col = vec3(0.);

    // Move camera
    vec3 cameraPos = vec3(sin(time*0.291)*0.5, 1., 6.);
    vec3 cameraFwd = vec3(0., 0., -1.);
    vec3 cameraUp = vec3(0., 1., 0.);
    vec3 cameraRight = vec3(1., 0., 0.);
    float zoom = 1.5+sin(time*0.347)*0.2;
    
    // Cast a ray
    vec3 rayOrigin = cameraPos + cameraFwd*zoom + cameraUp*uv.y + cameraRight*uv.x;
    vec3 rayDirection = normalize(rayOrigin-cameraPos);
    
    float d = CastRay(rayOrigin, rayDirection, time);
    vec3 point = rayOrigin + rayDirection * d;
    
    // move light 1 and shade fragment
 	vec3 lightPos = vec3(0., 0.5, 0.);
    lightPos.xz += vec2(sin(time), cos(time)) * 1.;
    col += Light(point, lightPos * 1.5, time) * vec3(1., 0., 0.);
    
    // move light 2 and shade fragment
    lightPos = vec3(0., 3., cos(time*0.87))*(3.+sin(time*2.));
    col += Light(point, lightPos, time) * vec3(1., 1., 0.);
    
    // ambient
    col += vec3(0., 0., 0.35);

	return col;
}

#endif