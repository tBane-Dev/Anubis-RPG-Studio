#version 330 core

// Water mesh: x, z = position on the map, y = terrain height under the vertex.

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightViewProjection;
uniform float uLevel;			// height of the water surface
uniform float time;

out vec2 worldPos;
out vec2 vTexCoord;
out float vDepth;
out vec4 vLightPos;

void main() {

    float level = uLevel;
    level -= 7.5 * (0.5 + 0.5 * sin(time * 0.8 - (aPosition.x + aPosition.z) * 0.02));

    vec4 position = vec4(aPosition.x, level, aPosition.z, 1.0);

    worldPos = aPosition.xz;
    vTexCoord = aTexCoord;
    vDepth = level - aPosition.y;
    vLightPos = uLightViewProjection * position;
    gl_Position = uProjection * uView * position;
}
