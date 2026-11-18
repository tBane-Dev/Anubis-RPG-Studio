#version 330 core

// Waters: flat surfaces separate from the terrain (ocean, swamp, lava).
// The vertex y of the mesh is the terrain height - the vertex shader puts the vertex on the water level
// and passes the difference as vDepth (0 on the shore, more in deep water).
// Ocean: waves and shore foam like in Catlike Coding "Hex Map 8" (noise texture, 4 channels).

in vec2 worldPos;
in vec2 vTexCoord;		// mask texture coords of this chunk
in float vDepth;
in vec4 vLightPos;
out vec4 fragColor;

uniform sampler2D uNoise;		// tileable noise, 4 independent channels
uniform sampler2D uShadowMap;
uniform sampler2D uMask;		// where the water is in this chunk
uniform int uWaterType;			// 0 = ocean, 1 = swamp, 2 = lava
uniform bool uSelected;
uniform float uShoreWidth;		// depth (world units) where the shore effects end
uniform float uShadowStrength;
uniform int uShadowSamples;

uniform vec2 resolution;
uniform float time;
uniform vec2 startPos;

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

// OCEAN /////////////////////////////////////////////////////////////////////////////////////////////////////////

// two moving noise samples, a slow sine wave chooses the channels - the pattern changes all the time
float waves(vec2 p) {
    vec2 uvA = vec2(p.x, p.y + time);
    vec4 noiseA = texture(uNoise, uvA * 0.025);

    vec2 uvB = vec2(p.x + time, p.y);
    vec4 noiseB = texture(uNoise, uvB * 0.025);

    float blendWave = sin((p.x + p.y) * 0.1 + (noiseA.y + noiseB.z) + time);
    blendWave *= blendWave;

    float w = mix(noiseA.z, noiseA.w, blendWave) + mix(noiseB.x, noiseB.y, blendWave);
    return smoothstep(0.75, 2.0, w);
}

// shore: 0 = open water, 1 = on the shore. Foam goes to the shore and back.
float foam(float shore, vec2 p) {
    shore = sqrt(shore) * 0.9;

    vec2 noiseUV = p + time * 0.25;
    vec4 noise = texture(uNoise, noiseUV * 0.015);

    float distortion1 = noise.x * (1.0 - shore);
    float foam1 = sin((shore + distortion1) * 10.0 - time);
    foam1 *= foam1;

    float distortion2 = noise.y * (1.0 - shore);
    float foam2 = sin((shore + distortion2) * 10.0 + time + 2.0);
    foam2 *= foam2 * 0.7;

    return max(foam1, foam2) * shore;
}

vec4 oceanColor(vec2 p) {
    float shore = 1.0 - clamp(vDepth / uShoreWidth, 0.0, 1.0);

    // shallow water is lighter and more transparent
    vec4 shallowColor = vec4(0.20, 0.45, 0.80, 0.45);
    vec4 deepColor = vec4(0.06, 0.18, 0.50, 0.85);
    vec4 color = mix(deepColor, shallowColor, shore);

    float foamValue = foam(shore, p);
    float waveValue = waves(p) * (1.0 - shore);

    // big slow swells - lighter and darker bands of blue
    vec4 swell = texture(uNoise, (p + vec2(time * 0.6, time * 0.3)) * 0.008);
    color.rgb *= 0.85 + 0.3 * swell.z;

    // crests: light blue, only the strongest ones go a bit to white
    color.rgb += vec3(0.30, 0.50, 0.70) * waveValue * 0.7;
    color.rgb += vec3(waveValue * waveValue * 0.25);

    // foam on the shore stays white
    color += vec4(foamValue);
    return clamp(color, 0.0, 1.0);
}

// BOG //////////////////////////////////////////////////////////////////////////////////////////////////////////
vec4 permute(vec4 x) {
    return mod((34.0 * x + 1.0) * x, 289.0);
}

vec3 permute(vec3 x) {
    return mod((34.0 * x + 1.0) * x, 289.0);
}

vec2 cellular2x2x2(vec3 P) {
	#define K 0.142857142857
	#define Ko 0.428571428571
	#define K2 0.020408163265306
	#define Kz 0.166666666667
	#define Kzo 0.416666666667
	#define jitter 0.8
	vec3 Pi = mod(floor(P), 289.0);
	vec3 Pf = fract(P);
	vec4 Pfx = Pf.x + vec4(0.0, -1.0, 0.0, -1.0);
	vec4 Pfy = Pf.y + vec4(0.0, 0.0, -1.0, -1.0);
	vec4 p = permute(Pi.x + vec4(0.0, 1.0, 0.0, 1.0));
	p = permute(p + Pi.y + vec4(0.0, 0.0, 1.0, 1.0));
	vec4 p1 = permute(p + Pi.z);
	vec4 p2 = permute(p + Pi.z + vec4(1.0));
	vec4 ox1 = fract(p1*K) - Ko;
	vec4 oy1 = mod(floor(p1*K), 7.0)*K - Ko;
	vec4 oz1 = floor(p1*K2)*Kz - Kzo;
	vec4 ox2 = fract(p2*K) - Ko;
	vec4 oy2 = mod(floor(p2*K), 7.0)*K - Ko;
	vec4 oz2 = floor(p2*K2)*Kz - Kzo;
	vec4 dx1 = Pfx + jitter*ox1;
	vec4 dy1 = Pfy + jitter*oy1;
	vec4 dz1 = Pf.z + jitter*oz1;
	vec4 dx2 = Pfx + jitter*ox2;
	vec4 dy2 = Pfy + jitter*oy2;
	vec4 dz2 = Pf.z - 1.0 + jitter*oz2;
	vec4 d1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
	vec4 d2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;

	vec4 d = min(d1,d2);
	d2 = max(d1,d2);
	d.xy = (d.x < d.y) ? d.xy : d.yx;
	d.xz = (d.x < d.z) ? d.xz : d.zx;
	d.xw = (d.x < d.w) ? d.xw : d.wx;
	d.yzw = min(d.yzw, d2.yzw);
	d.y = min(d.y, d.z);
	d.y = min(d.y, d.w);
	d.y = min(d.y, d2.x);
	return sqrt(d.xy);
}

vec3 bogColor() {
    float pixelSize = 2.0;
    vec2 pixelatedWorldPos = floor(worldPos / pixelSize) * pixelSize;
    vec2 st = (2.0 * (pixelatedWorldPos - startPos) - resolution.xy) / resolution.y;
    st.y = st.y * 2.0;
    st *= 10.0;
    vec2 F = cellular2x2x2(vec3(st, time / 8.0));
    float n = smoothstep(0.4, 0.5, F.x);

    if (n < 0.6)
        return vec3(0.15, 0.35, 0.15);

    return vec3(0.15 * n, 0.25 * n, 0.15 * n);
}
// LAVA //////////////////////////////////////////////////////////////////////////////////////////////////////////

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

float lavaNoise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
                     dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
                     dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

vec3 lavaFunc(vec3 color, vec2 uv, float detail, float power,
              float colorMul, float glowRate, bool animate, float noiseAmount)
{
    vec3 rockColor = vec3(0.09 + abs(sin(time * .75)) * .03, 0.02, .02);
    float minDistance = 1.;
    uv *= detail;
    uv *= 10.0;

    vec2 cell = floor(uv);
    vec2 frac = fract(uv);

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 cellDir = vec2(float(i), float(j));
            vec2 randPoint = random2(cell + cellDir);
            randPoint += lavaNoise(uv) * noiseAmount;
            randPoint = animate ? 0.5 + 0.5 * sin(time * .35 + 6.2831 * randPoint) : randPoint;
            minDistance = min(minDistance, length(cellDir + randPoint - frac));
        }
    }

    float powAdd = sin(uv.x * 2. + time * glowRate) + sin(uv.y * 2. + time * glowRate);
    vec3 outColor = vec3(color * pow(minDistance, power + powAdd * .95) * colorMul);
    outColor.rgb = mix(rockColor, outColor.rgb, minDistance);
    return outColor;
}

vec3 lavaColor() {
    vec2 uv = (worldPos.xy - startPos) / resolution;
    uv.x *= resolution.x / resolution.y;
    float pixelSize = 4.0;
    vec2 grid = pixelSize / resolution;
    uv = floor(uv / grid) * grid;
    vec3 color = vec3(0.);
    float addPower = -1.5;
    color += lavaFunc(vec3(1.5, .45, 0.), uv*2, 3.,  2.5+addPower, 1.15, 1.5, false, 1.5);
    color += lavaFunc(vec3(1.5, 0., 0.), uv*2, 6., 3.+addPower, .4, 1., false, 0.);
    color += lavaFunc(vec3(1.2, .4, 0.), uv*2, 8., 4.+addPower, .2, 1.9, true, 0.5);
    return color;
}
// MAIN /////////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    if (texture(uMask, vTexCoord).r < 0.5 || vDepth <= 0.0)
        discard;

    // one step per world unit - pixelated like the terrain
    vec2 p = floor(worldPos) + 0.5;

    vec4 color;

    if (uWaterType == 1)
        color = vec4(bogColor(), 0.92);
    else if (uWaterType == 2)
        color = vec4(lavaColor(), 1.0);
    else
        color = oceanColor(p * 0.6);	// 0.6: scale of the Hex Map waves to our tiles

    // swamp and lava: soft edge on the shore
    if (uWaterType != 0)
        color.a *= clamp(vDepth / 2.0, 0.0, 1.0);

    if (uSelected)
        color.rgb += vec3(0.12) * (0.5 + 0.5 * sin(time * 4.0));

    color.rgb *= 1.0 - uShadowStrength * shadowAmount();
    fragColor = color;
}
