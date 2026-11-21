#version 330 core

// Terrain brush drawn on the terrain geometry (vertex shader: terrain3D.vert).
// Soft circle / square: the fill is strongest in the middle, the outline shows the whole size.
// uCenter != uEnd: the brush is drawn along the line from uCenter to uEnd (slope tool).

in vec2 worldPos;
in vec2 vTexCoord;

out vec4 fragColor;

uniform vec2 uCenter;
uniform vec2 uEnd;
uniform float uRadius;
uniform bool uSquare;
uniform vec4 uColor;
uniform float uLineWidth;	// outline width in world units

void main() {
    // the nearest point of the line (for uCenter == uEnd it is uCenter)
    vec2 line = uEnd - uCenter;
    float t = clamp(dot(worldPos - uCenter, line) / max(dot(line, line), 0.0001), 0.0, 1.0);
    vec2 delta = worldPos - (uCenter + line * t);
    float dist = uSquare ? max(abs(delta.x), abs(delta.y)) : length(delta);

    if (dist > uRadius)
        discard;

    // the same falloff as Map::getBrushFalloff()
    float f = clamp((uRadius - dist) / (uRadius * 0.5), 0.0, 1.0);
    f = f * f * (3.0 - 2.0 * f);

    if (uRadius - dist < uLineWidth)
        fragColor = vec4(uColor.rgb, min(1.0, uColor.a * 2.0));
    else
        fragColor = vec4(uColor.rgb, uColor.a * (0.25 + 0.75 * f));
}
