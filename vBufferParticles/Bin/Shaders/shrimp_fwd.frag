#version 450
#extension GL_ARB_separate_shader_objects : enable

/// Fragment shader for static meshes with Shrimp texture applied in Forward renderer.

// reads texture bound at location 1.
#define TEXTURE_BINDING 1
#include "default_fwd.glsl"
