// sprite2.5D.vert

uniform vec2 cameraCenter;
uniform float cameraDistance;
uniform float cameraAngle;
uniform float aspect;

uniform vec2 spritePosition;
uniform vec2 spriteScale; 

void main()
{
    float angle = radians(cameraAngle);
    float s = sin(angle);
    float c = cos(angle);

    vec2 position = spritePosition - cameraCenter;

    vec3 anchor = vec3(
        position.x,
        position.y * c,
        position.y * s
    );

    vec2 local = (gl_ModelViewMatrix * gl_Vertex).xy * spriteScale;

    vec2 screenPosition = anchor.xy + local;

    float depth = cameraDistance - anchor.z;

    float f = 1.7320508; // 1 / tan(60 stopni / 2)

    float nearPlane = 0.1;
    float farPlane = 100000.0;

    float clipZ = ((farPlane + nearPlane) / (farPlane - nearPlane)) * depth - (2.0 * farPlane * nearPlane) / (farPlane - nearPlane);

    gl_Position = vec4(
        screenPosition.x * f / aspect ,
        -screenPosition.y * f,
        clipZ,
        depth
    );

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}