#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Fragment shader for static meshes with Raccoon texture applied in G-Buffer (6) renderer.

// reads texture bound at location 2.
#define TEXTURE_BINDING 2
#include "default_g.glsl"
