#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform bool uSkinned;
uniform samplerBuffer uJointMatrices;
uniform vec4 uWind;   // x = strength, y = time

out vec2 vTexCoord;

mat4 getJointMatrix(uint jointIndex) {
    int offset = int(jointIndex) * 4;

    // Each matrix is stored as four column vectors.
    return mat4(
        texelFetch(uJointMatrices, offset),
        texelFetch(uJointMatrices, offset + 1),
        texelFetch(uJointMatrices, offset + 2),
        texelFetch(uJointMatrices, offset + 3)
    );
}

vec3 applyWind(vec3 p) {
    if (uWind.x <= 0.0)
        return p;

    // different phase for every model, from its world position
    float phase = uModel[3].x * 0.013 + uModel[3].z * 0.017;
    float t = uWind.y;

    // higher vertex = bigger move, the bottom stays in the ground
    float h = max(p.y, 0.0);
    float bend = h * h * uWind.x;

    float wave = sin(t * 1.6 + phase) + 0.4 * sin(t * 3.7 + phase * 2.3 + p.x * 4.0);

    p.x += wave * bend;
    p.z += 0.5 * wave * bend;
    return p;
}

void main() {
    mat4 skinMatrix = mat4(1.0);

    if (uSkinned) {
        float totalWeight =
            aWeights.x +
            aWeights.y +
            aWeights.z +
            aWeights.w;

        if (totalWeight > 0.0) {
            vec4 weights = aWeights / totalWeight;

            skinMatrix = mat4(0.0);

            // Only access joints that influence this vertex.
            for (int i = 0; i < 4; ++i) {
                if (weights[i] > 0.0) {
                    skinMatrix += getJointMatrix(aJoints[i]) * weights[i];
                }
            }
        }
    }

    vec4 position = skinMatrix * vec4(aPosition, 1.0);
    position.xyz = applyWind(position.xyz);

    gl_Position = uProjection * uView * uModel * position;
    vTexCoord = aTexCoord;
}