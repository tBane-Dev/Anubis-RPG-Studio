// vertex3D.vert

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 position;

varying vec2 worldPos;

void main()
{
    vec2 mapPosition = gl_Vertex.xy + position;

    worldPos = mapPosition;

    vec4 worldPosition = vec4(
        mapPosition.x,
        0.0,
        mapPosition.y,
        1.0
    );

    gl_Position = uProjection * uView * worldPosition;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}