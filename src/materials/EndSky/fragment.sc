#ifndef INSTANCING
$input v_texcoord0, v_posTime
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  SAMPLER2D_AUTOREG(s_SkyTexture);
#endif

uniform vec4 ViewPositionAndTime;


void main() {
  #ifndef INSTANCING
    vec4 diffuse = texture2D(s_SkyTexture, v_texcoord0);

    // Time-based rotation for the starfield
    highp float time = ViewPositionAndTime.w;

    // Define rotation speed and axis
    float rotationSpeed = 0.1; // Adjust this value for faster/slower rotation
    vec3 rotationAxis = vec3(0.0, 1.0, 0.0); // Y-axis rotation (adjust as needed)

    // Apply circular rotation around the center
    float angle = rotationSpeed * time;
    mat3 rotationMatrix = mat3(
        cos(angle), 0.0, -sin(angle),
        0.0, 1.0, 0.0,
        sin(angle), 0.0, cos(angle)
    );

    // Calculate the starfield position with rotation
    vec3 rotatedPos = rotationMatrix * normalize(v_posTime.xyz);

    // Starfield pattern (simple dot product to simulate stars)
    float starField = max(0.0, dot(rotatedPos, vec3(0.0, 0.0, 1.0))); // Focused on the Z-axis
    starField = pow(starField, 50.0); // Make stars sharper and smaller

    // Blend starfield with the sky color
    vec3 color = renderEndSky(getEndHorizonCol(), getEndZenithCol(), rotatedPos, time);
    color += 2.8 * diffuse.rgb; // Stars from texture
    color += vec3(starField); // Procedural stars

    // Final color correction
    color = colorCorrection(color);

    gl_FragColor = vec4(color, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
