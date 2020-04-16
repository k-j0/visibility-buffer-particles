#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Fragment shader for static meshes with Raccoon texture applied in Forward renderer.

// reads texture bound at location 3.
#define TEXTURE_BINDING 3
#include "default_fwd.glsl"
