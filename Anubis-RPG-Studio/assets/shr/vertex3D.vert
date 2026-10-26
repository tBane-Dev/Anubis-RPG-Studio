// vertex3D.vert

uniform vec2 cameraCenter;
uniform float cameraDistance;
uniform float cameraAngle;
uniform float aspect;
uniform vec2 position;

varying vec2 worldPos;

void main()
{
    float angle = radians(cameraAngle);
    float s = sin(angle);
    float c = cos(angle);

    vec4 pos = gl_Vertex;
    pos.xy += position;

    worldPos = pos.xy;
    pos.xy -= cameraCenter;

    vec3 rotated = vec3(
        pos.x,
        pos.y * c,
        pos.y * s
    );

    float depth = cameraDistance - rotated.z;
    float f = 1.7320508;

    float nearPlane = 0.1;
    float farPlane = 100000.0;

    float clipZ =
        ((farPlane + nearPlane) / (farPlane - nearPlane)) * depth
        - (2.0 * farPlane * nearPlane) / (farPlane - nearPlane);

    gl_Position = vec4(
        rotated.x * f / aspect,
        -rotated.y * f,
        clipZ,
        depth
    );

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}