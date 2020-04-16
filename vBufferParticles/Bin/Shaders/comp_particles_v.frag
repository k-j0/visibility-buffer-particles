#version 450

/// Fragment shader for particles using comp/comp in V-Buffer pipeline

#define COMP_PARTICLE_FRAGMENT // <- set this flag to denote the use of comp/comp as it influences where textures are bound
#include "particles_v_frag_firstpass.glsl" // use generic v-buffer particle fragment shader
