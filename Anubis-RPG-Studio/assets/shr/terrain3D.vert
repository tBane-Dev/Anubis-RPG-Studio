#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightViewProjection;	// world -> shadow map (Scene::renderShadowMap)

out vec2 worldPos;
out vec2 vTexCoord;
out vec4 vLightPos;

void main() {
    worldPos = aPosition.xz;
    vTexCoord = aTexCoord;
    vLightPos = uLightViewProjection * vec4(aPosition, 1.0);
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
}
