#ifndef WATER_H
#define WATER_H

#include "constants.h"
#include "sky.h"
#include "clouds.h"
#include "noise.h"

// Fresnel - Schlick's approximation
float calculateFresnel(float cosR, float r0) {
  float a = 1.0 - cosR;
  float a2 = a * a;
  return r0 + (1.0 - r0) * a2 * a2 * a;
}

vec4 nlWater(
  inout vec3 wPos, inout vec4 color, vec4 COLOR, vec3 viewDir, vec3 light, vec3 cPos, vec3 tiledCpos,
  float fractCposY, vec3 FOG_COLOR, vec3 horizonCol, vec3 horizonEdgeCol, vec3 zenithCol,
  vec2 lit, highp float t, float camDist, float rainFactor,
  vec3 torchColor, bool end, bool nether, bool underWater
) {

  float cosR;
  float rippleFrequency1 = 0.9; // Frequency of first ripple wave
  float rippleAmplitude1 = 1.6;  // Amplitude of first ripple wave
  float rippleFrequency2 = 0.8; // Frequency of second ripple wave
  float rippleAmplitude2 = 4.0;  // Amplitude of second ripple wave
  float bumpBase = 0.15;         // Base bump value
  float bump = bumpBase + 0.05 * sin(t * 1.5); // Dynamic bump based on time
  vec3 waterRefl;

  if (fractCposY > 0.0) { // Reflection for top plane
    // Bump map using a combination of sine waves for more complex wave patterns
    float rippleEffect1 = rippleAmplitude1 * sin(t * rippleFrequency1 + wPos.x * 0.1 + wPos.z * 0.1);
    float rippleEffect2 = rippleAmplitude2 * sin(t * rippleFrequency2 + wPos.x * 0.2 + wPos.z * 0.2);
    float rippleEffect = rippleEffect1 + rippleEffect2;
    bump *= disp(tiledCpos, t) + rippleEffect;

    // Calculate cosine of incidence angle and apply water bump
    cosR = abs(viewDir.y);
    cosR = mix(cosR, 1.0 - cosR * cosR, bump);
    viewDir.y = cosR;

    // Sky reflection
    waterRefl = getSkyRefl(horizonEdgeCol, horizonCol, zenithCol, viewDir, FOG_COLOR, t, -wPos.y, rainFactor, end, underWater, nether);

    // Cloud and aurora reflection
    #if defined(NL_WATER_CLOUD_REFLECTION)
      if (wPos.y < 0.0) {
        vec2 parallax = viewDir.xz / viewDir.y;
        vec2 projectedPos = wPos.xz - parallax * 100.0 * (1.0 - bump);
        float fade = clamp(2.0 - 0.004 * length(projectedPos), 0.0, 1.0);

        #ifdef NL_AURORA
          vec4 aurora = renderAurora(projectedPos.xyy, t, rainFactor, FOG_COLOR);
          waterRefl += 2.0 * aurora.rgb * aurora.a * fade;
        #endif

        #if NL_CLOUD_TYPE == 1
          vec4 clouds = renderCloudsSimple(projectedPos.xyy, t, rainFactor, zenithCol, horizonCol, horizonEdgeCol);
          waterRefl = mix(waterRefl, 1.5 * clouds.rgb, clouds.a * fade);
        #endif
      }
    #endif

#ifdef MOONLIGHT
 // Add moonlight reflection effect (fake)
        float moonlightFactor = clamp(dot(viewDir, normalize(vec3(0.5, 0.5, 0.5))), 0.0, 1.0);
        waterRefl += NL_MOONLIGHT_COLOR * moonlightFactor * NL_MOONLIGHT_INTENSITY;
        #endif
         #ifdef SPECULAR
        //specular
    vec3 viewDirNorm = normalize(viewDir);
    vec3 reflDir = reflect(viewDirNorm, vec3(0, 1, 0));
    float specular = pow(max(dot(reflDir, viewDirNorm), 0.0), 16.0);
    specular *= 2.5; // Adjust specular intensity

    // Add specular highlight to water reflection
    waterRefl += specular * vec3(1.0, 1.0, 1.0);
    
    #endif
    // Torch light reflection
    waterRefl += torchColor * NL_TORCH_INTENSITY * (lit.x * lit.x + lit.x) * bump * 10.0;

    if (fractCposY > 0.8 || fractCposY < 0.9) { // Flat plane
      waterRefl *= 1.0 - clamp(wPos.y, 0.0, 0.66);
    } else { // Slanted plane and highly slanted plane
      waterRefl *= 0.1 * sin(t * 2.0 + cPos.y * 12.566) + (fractCposY > 0.9 ? 0.2 : 0.4);
    }
  } else { // Reflection for side plane
    bump *= 0.5 + 0.5 * sin(1.5 * t + dot(cPos, vec3_splat(NL_CONST_PI_HALF)));

    cosR = max(sqrt(dot(viewDir.xz, viewDir.xz)), step(wPos.y, 0.5));
    cosR += (1.0 - cosR * cosR) * bump;

    waterRefl = zenithCol;
  }

  // Mask sky reflection under shade
  if (!end) {
    waterRefl *= 0.05 + lit.y * 1.14;
  }

  float fresnel = calculateFresnel(cosR, 0.03);
  float opacity = 1.0 - cosR;

  color.rgb *= 0.22 * NL_WATER_TINT * (1.0 - 0.8 * fresnel);

  #ifdef NL_WATER_FOG_FADE
    color.a *= NL_WATER_TRANSPARENCY;
  #else
    color.a = COLOR.a * NL_WATER_TRANSPARENCY;
  #endif

  color.a += (1.0 - color.a) * opacity * opacity;

  #ifdef NL_WATER_WAVE
    if (camDist < 0.0) {
      wPos.y -= bump;
    }
  #endif

  return vec4(waterRefl, fresnel);
}

#endif
