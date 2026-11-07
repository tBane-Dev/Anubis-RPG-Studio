// sprite2.5D.vert

uniform mat4 uView;
uniform mat4 uProjection;

uniform vec2 spritePosition;
uniform vec2 spriteScale;

void main()
{
    vec4 anchor = uView * vec4(
        spritePosition.x,
        0.0,
        spritePosition.y,
        1.0
    );

    vec2 local = (gl_ModelViewMatrix * gl_Vertex).xy * spriteScale;

    // SFML: Y w dół. Przestrzeń kamery: Y w górę.
    anchor.xy += vec2(local.x, -local.y);

    gl_Position = uProjection * anchor;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}