#version 330 core

// Tile overlay drawn on the terrain geometry (vertex shader: terrain3D.vert).
// Used for the terrain brush and for the tile under a placed prefab.
// Every tile of the mask is filled with uColor and outlined, so the terrain grid is visible.

in vec2 worldPos;
in vec2 vTexCoord;

out vec4 fragColor;

uniform float uTileSize;
uniform vec2 uFirstTile;		// global tile coords of mask[0][0]
uniform int uWidth;
uniform int uHeight;
uniform float uMask[441];		// uWidth * uHeight values, 1 = tile is marked (max 21 x 21)
uniform vec4 uColor;
uniform float uGridWidth;		// outline width in world units

void main() {
    vec2 tile = floor(worldPos / uTileSize);
    ivec2 local = ivec2(tile - uFirstTile);

    if (local.x < 0 || local.y < 0 || local.x >= uWidth || local.y >= uHeight)
        discard;

    if (uMask[local.y * uWidth + local.x] < 0.5)
        discard;

    vec2 inTile = worldPos - tile * uTileSize;
    float edge = min(min(inTile.x, uTileSize - inTile.x), min(inTile.y, uTileSize - inTile.y));

    if (edge < uGridWidth)
        fragColor = vec4(uColor.rgb, min(1.0, uColor.a * 2.0));
    else
        fragColor = uColor;
}
