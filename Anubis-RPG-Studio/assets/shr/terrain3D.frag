#version 330 core

// Smooth terrain with texture splatting.
// Every chunk has 2 splat textures 65 x 65 (one texel = one weight point, every 4 world units):
//   uSplat0.rgba = weights of terrains 0-3, uSplat1.rgba = weights of terrains 4-7.
// Linear filtering gives smooth weights, height blending (+ noise) makes the border sharp and uneven.

in vec2 vTexCoord;		// splat texture coords of this chunk
in vec2 worldPos;
in vec3 vNormal;
in vec4 vLightPos;
out vec4 fragColor;

uniform sampler2D uTerrainTexture;	// tileset.png: all terrains in one column (size x size each, e.g. 64 or 256)
uniform sampler2D uSplat0;
uniform sampler2D uSplat1;
uniform int uTerrainCount;			// terrains in the atlas (8)
uniform float uTextureWorldSize;	// one terrain texture covers this many world units
uniform float uSplatWorldSize;		// world size of the whole splat texture (65 texels * 4)
uniform float uTileSize;
uniform vec3 uSunDirection;

uniform sampler2D uShadowMap;
uniform float uShadowStrength;	// 0 = no shadows, 1 = black shadows
uniform int uShadowSamples;		// 1 = one sample, 3 = 3x3

#define SEA_BOTTOM 0
#define BEACH 1
#define GRASS 2
#define SWAMP 3
#define SWAMP_BOTTOM 4
#define HIGHLAND 5
#define MAGMA 6
#define LAVA_ROCK 7

// SHADOWS ///////////////////////////////////////////////////////////////////////////////////////////////////////

// 0 = lit, 1 = fully in shadow. 3x3 samples soften the edge.
float shadowAmount() {
    vec3 p = vLightPos.xyz / vLightPos.w * 0.5 + 0.5;

    if (p.x <= 0.0 || p.x >= 1.0 || p.y <= 0.0 || p.y >= 1.0 || p.z >= 1.0)
        return 0.0;

    const float bias = 0.0005;

    if (uShadowSamples <= 1)
        return (p.z - bias > texture(uShadowMap, p.xy).r) ? 1.0 : 0.0;

    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++)
            shadow += (p.z - bias > texture(uShadowMap, p.xy + vec2(x, y) * texel).r) ? 1.0 : 0.0;

    return shadow / 9.0;
}

// NOISE ////////////////////////////////////////////////////////////////////////////////////////////////////////

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

// SPLATTING /////////////////////////////////////////////////////////////////////////////////////////////////////

// Pixel of one terrain from the atlas. p = world position (center of a texel).
vec3 atlasColor(int id, vec2 p) {
    vec2 inTexture = fract(p / uTextureWorldSize);
    vec2 uv = vec2(inTexture.x, (float(id) + inTexture.y) / float(uTerrainCount));
    return texture(uTerrainTexture, uv).rgb;
}

// gradient noise, about -0.7 .. 0.7, average 0
float borderNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = dot(random2(i + vec2(0.0, 0.0)) * 2.0 - 1.0, f - vec2(0.0, 0.0));
    float b = dot(random2(i + vec2(1.0, 0.0)) * 2.0 - 1.0, f - vec2(1.0, 0.0));
    float c = dot(random2(i + vec2(0.0, 1.0)) * 2.0 - 1.0, f - vec2(0.0, 1.0));
    float d = dot(random2(i + vec2(1.0, 1.0)) * 2.0 - 1.0, f - vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// How "high" a terrain lies on the border. Higher terrain covers lower terrain
// (beach covers water, highland covers grass ...). Texture brightness and noise make the border uneven.
float terrainHeight(int id, vec3 color, vec2 p) {
    float base[8] = float[8](
        0.00,	// sea bottom (sand)
        0.35,	// beach
        0.45,	// grass
        0.30,	// swamp
        0.10,	// swamp bottom
        0.60,	// highland
        0.40,	// magma (dry lava)
        0.05	// lava rock
    );

    float detail = dot(color, vec3(0.299, 0.587, 0.114));
    vec2 q = p / uTileSize + vec2(float(id) * 7.31, float(id) * 3.17);
    float noise = borderNoise(q * 0.6) + borderNoise(q * 1.9) * 0.4;
    return base[id] + detail * 0.5 + noise * 0.9;
}

// MAIN /////////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    // one blend step per texel - the borders stay pixelated like the textures
    float texelWorld = uTextureWorldSize / float(textureSize(uTerrainTexture, 0).x);
    vec2 pixel = (floor(worldPos / texelWorld) + 0.5) * texelWorld;

    vec2 splatSize = vec2(textureSize(uSplat0, 0));
    vec2 splatUV = vTexCoord + (pixel - worldPos) / uSplatWorldSize;
    splatUV = clamp(splatUV, 0.5 / splatSize, 1.0 - 0.5 / splatSize);

    vec4 w0 = texture(uSplat0, splatUV);
    vec4 w1 = texture(uSplat1, splatUV);
    float weights[8] = float[8](w0.r, w0.g, w0.b, w0.a, w1.r, w1.g, w1.b, w1.a);

    // 1. height of every terrain under this pixel
    vec3 colors[8];
    float heights[8];
    float highest = -10.0;

    for (int i = 0; i < 8; i++) {
        colors[i] = vec3(0.0);
        heights[i] = -10.0;

        if (weights[i] < 0.004)
            continue;

        colors[i] = atlasColor(i, pixel);

        heights[i] = weights[i] + terrainHeight(i, colors[i], pixel);
        highest = max(highest, heights[i]);
    }

    // 2. height blending - only terrains close to the highest one are visible
    const float blendDepth = 0.08;
    vec3 color = vec3(0.0);
    float total = 0.0;

    for (int i = 0; i < 8; i++) {
        float b = max(heights[i] - (highest - blendDepth), 0.0);

        if (b <= 0.0)
            continue;

        color += colors[i] * b;
        total += b;
    }

    color /= max(total, 0.0001);

    // light: flat ground keeps its color, slopes to the sun are brighter, the others darker
    vec3 toSun = -normalize(uSunDirection);
    float flatLight = max(toSun.y, 0.05);
    float light = max(dot(normalize(vNormal), toSun), 0.0) / flatLight;
    color *= clamp(mix(1.0, light, 0.85), 0.35, 1.3);

    fragColor = vec4(color, 1.0);
    fragColor.rgb *= 1.0 - uShadowStrength * shadowAmount();
}
