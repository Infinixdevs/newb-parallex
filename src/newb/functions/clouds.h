#ifndef CLOUDS_H
#define CLOUDS_H

#include "noise.h"

// simple clouds 2D noise
float cloudNoise2D(vec2 p, highp float t, float rain) {
  t *= NL_CLOUD1_SPEED;
  p += t;
  p.x += sin(p.y*0.4 + t);

  vec2 p0 = floor(p);
  vec2 u = p-p0;
  u *= u*(3.0-2.0*u);
  vec2 v = 1.0-u;

  // rain transition
  vec2 d = vec2(0.09+0.5*rain,0.089+0.5*rain*rain);

  return v.y*(randt(p0,d)*v.x + randt(p0+vec2(1.0,0.0),d)*u.x) +
         u.y*(randt(p0+vec2(0.0,1.0),d)*v.x + randt(p0+vec2(1.0,1.0),d)*u.x);
}

// simple clouds
vec4 renderCloudsSimple(vec3 pos, highp float t, float rain, vec3 zenithCol, vec3 horizonCol, vec3 fogCol) {
  pos.xz *= NL_CLOUD1_SCALE;

  float cloudAlpha = cloudNoise2D(pos.xz, t, rain);
  float cloudShadow = cloudNoise2D(pos.xz*0.91, t, rain);

  vec4 color = vec4(0.02,0.04,0.05,cloudAlpha);

  color.rgb += fogCol;
  color.rgb *= 1.0 - 0.5*cloudShadow*step(0.0, pos.y);

  color.rgb += zenithCol*0.7;
  color.rgb *= 1.0 - 0.4*rain;

  return color;
}
#if TYPEN == 1

// Hash function to generate pseudo-random values
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Simple noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

// Fractal Brownian Motion (FBM) with a variable number of steps
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;

    for (int i = 0; i < STEPS; i++) {
        value += amplitude * noise(p);
        p *= 0.5;
        amplitude *= 0.5;
    }

    return value;
}

// rounded clouds
float cloudDf(vec3 pos, float rain) {
    vec2 p0 = floor(pos.xz);
    vec2 u = smoothstep(0.999*NL_CLOUD2_SHAPE, 1.0, pos.xz - p0);

    // Rain transition
    vec2 t = vec2(0.1001 + 0.2 * rain, 0.1 + 0.2 * rain * rain);

    // Use FBM noise for the cloud density map
    float n = mix(
        mix(fbm(p0 * 0.1), fbm((p0 + vec2(1.0, 0.0)) * 0.1), u.x),
        mix(fbm((p0 + vec2(0.0, 1.0)) * 0.1), fbm((p0 + vec2(1.0, 1.0)) * 0.1), u.x),
        u.y
    );
    
    // Round y
    float b = 1.0 - 1.9 * smoothstep(NL_CLOUD2_SHAPE, 2.0 - NL_CLOUD2_SHAPE, 2.0 * abs(pos.y - 0.5));
    return smoothstep(0.2, 1.0, n * b);
}

vec4 renderClouds(vec3 vDir, vec3 vPos, float rain, float time, vec3 fogCol, vec3 skyCol) {

  float height = 37.0*mix(NL_CLOUD2_THICKNESS, NL_CLOUD2_RAIN_THICKNESS, rain);

  // scaled ray offset
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = (NL_CLOUD2_SCALE*height)*vDir.xz/(0.02+0.98*abs(vDir.y));
  //deltaP.xyz = (NL_CLOUD2_SCALE*height)*vDir.xyz;
  //deltaP.y = abs(deltaP.y);

  // local cloud pos
  vec3 pos;
  pos.y = 0.0;
  pos.xz = NL_CLOUD2_SCALE*(vPos.xz + vec2(1.0,0.5)*(time*NL_CLOUD2_VELOCIY));
  pos += deltaP;

  deltaP /= -float(NL_CLOUD2_STEPS);

  // alpha, gradient
  vec2 d = vec2(0.0,1.0);
  for (int i=1; i<=NL_CLOUD2_STEPS; i++) {
    float m = cloudDf(pos, rain);

    d.x += m;
    d.y = mix(d.y, pos.y, m);

    //if (d.x == 0.0 && i > NL_CLOUD2_STEPS/2) {
    //        break;
    //} 
    pos += deltaP;
  }
  //d.x *= vDir.y*vDir.y;
  d.x *= smoothstep(0.03, 0.1, d.x);
  d.x = d.x / ((float(NL_CLOUD2_STEPS)/NL_CLOUD2_DENSITY) + d.x);

  if (vPos.y > 0.0) { // view from bottom
    d.y = 1.0 - d.y;
  }

  d.y = 1.0 - NL_CLOUDS_SHADOW_INTENSITY*d.y*d.y;

  float night = max(1.1 - 3.0*max(fogCol.b, fogCol.g), 0.0);

  vec4 col = vec4(0.4*skyCol, d.x);
  col.rgb += (skyCol + NL_CLOUDS_BRIGHTNESS*fogCol)*d.y;
  col.rgb *= 1.0 - 0.5*rain;
  col.rgb *= 1.0 - 0.8*night;

    #ifdef NL_CLOUDS_INTENSITY
    float op = col.a - d.y;
    op = col.a - d.x;

    op = NL_CLOUDS_INTENSITY;

    col.a *= op;
    #endif
  return col;
}
#elif TYPEN == 2
#define SCALE 3.0
#define ANIMATION_SPEED 0.1
#define SCALE 3.0

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

// rounded clouds 3D density map
float cloudDf(vec3 pos, float rain) {
    vec2 p0 = floor(pos.xz);
    vec2 u = smoothstep(0.999 * NL_CLOUD2_SHAPE, 1.0, pos.xz - p0);

    // rain transition
    vec2 t = vec2(0.1001 + 0.2 * rain, 0.1 + 0.2 * rain * rain);

    float n = mix(
        mix(randt(p0, t), randt(p0 + vec2(1.0, 0.0), t), u.x),
        mix(randt(p0 + vec2(0.0, 1.0), t), randt(p0 + vec2(1.0, 1.0), t), u.x),
        u.y
    );

    // Introduce static noise
    vec2 noisePos = pos.xz * SCALE;
    float cloudNoise = noise(noisePos);

    // round y
    float b = 1.0 - 1.9 * smoothstep(NL_CLOUD2_SHAPE, 2.0 - NL_CLOUD2_SHAPE, 2.0 * abs(pos.y - 0.5));
    return smoothstep(0.2, 1.0, n * b * (0.8 + 0.2 * cloudNoise));
}

vec4 renderClouds(vec3 vDir, vec3 vPos, float rain, float time, vec3 fogCol, vec3 skyCol) {
    float height = 13.0 * mix(NL_CLOUD2_THICKNESS, NL_CLOUD2_RAIN_THICKNESS, rain);

    // scaled ray offset
    vec3 deltaP;
    deltaP.y = 1.0;
    deltaP.xz = (NL_CLOUD2_SCALE * height) * vDir.xz / (0.02 + 0.98 * abs(vDir.y));

    // local cloud pos
    vec3 pos;
    pos.y = 0.0;
    pos.xz = NL_CLOUD2_SCALE * (vPos.xz + vec2(1.0, 0.5) * (time * NL_CLOUD2_VELOCIY));
    pos += deltaP;

    deltaP /= -float(NL_CLOUD2_STEPS);

    // alpha, gradient
    vec2 d = vec2(0.0, 1.0);
    for (int i = 1; i <= NL_CLOUD2_STEPS; i++) {
        float m = cloudDf(pos, rain);

        d.x += m;
        d.y = mix(d.y, pos.y, m);

        pos += deltaP;
    }

    d.x *= smoothstep(0.03, 0.1, d.x);
    d.x = d.x / ((float(NL_CLOUD2_STEPS) / NL_CLOUD2_DENSITY) + d.x);

    if (vPos.y > 0.0) { // view from bottom
        d.y = 1.0 - d.y;
    }

    d.y = 1.0 - 0.7 * d.y * d.y;

    vec4 col = vec4(0.6 * skyCol, d.x);
    col.rgb += (vec3(0.03, 0.05, 0.05) + 0.8 * fogCol) * d.y;
    col.rgb *= 1.0 - 0.5 * rain;

    return col;
}
#elif TYPEN == 3
// renamed_rand function
highp float renamed_rand(highp vec2 x) {
    return fract(sin(dot(x, vec2(14.56, 56.2))) * 20000.);
}

// Noise function based on renamed_rand
highp float noise(highp vec2 x) {
    highp vec2 ipos = floor(x);
    highp vec2 fpos = fract(x);
    fpos = smoothstep(0., 1., fpos);
    float a = renamed_rand(ipos), b = renamed_rand(ipos + vec2(1, 0)),
        c = renamed_rand(ipos + vec2(0, 1)), d = renamed_rand(ipos + 1.);
    return mix(a, b, fpos.x) + (c - a) * fpos.y * (1.0 - fpos.x) + (d - b) * fpos.x * fpos.y;
}

// Updated sildur function without time dependency
float sildur(vec2 uv, float c, float seed) {
    float a = 1.7;
    float b = 0.1;
    const int pvp = 3;
    vec2 new_uv = uv + vec2(seed, seed);  // Using seed instead of time
    for (int lop = 0; lop < pvp; lop++) {
        b += noise(new_uv) / a / c;
        a *= 3.;
        new_uv *= 3.;
    }
    return 1.0 - pow(0.1, max(0.8 - b, 0.0));
}

// cloudDf function
float cloudDf(vec3 pos, float rain, float seed) {
    vec2 p0 = floor(pos.xz);
    vec2 u = smoothstep(0.999 * NL_CLOUD2_SHAPE, 1.0, pos.xz - p0);

    // rain transition
    vec2 t = vec2(0.1001 + 0.2 * rain, 0.1 + 0.2 * rain * rain);

    float n = mix(
        mix(noise(p0), noise(p0 + vec2(1.0, 0.0)), u.x),
        mix(noise(p0 + vec2(0.0, 1.0)), noise(p0 + vec2(1.0, 1.0)), u.x),
        u.y
    );

    // round y
    float b = 1.0 - 1.9 * smoothstep(NL_CLOUD2_SHAPE, 2.0 - NL_CLOUD2_SHAPE, 2.0 * abs(pos.y - 0.5));
    return smoothstep(0.2, 1.0, n * b);
}

// renderClouds function
vec4 renderClouds(vec3 vDir, vec3 vPos, float rain, float seed, vec3 fogCol, vec3 skyCol) {
    float height = 7.0 * mix(NL_CLOUD2_THICKNESS, NL_CLOUD2_RAIN_THICKNESS, rain);

    // scaled ray offset
    vec3 deltaP;
    deltaP.y = 1.0;
    deltaP.xz = (NL_CLOUD2_SCALE * height) * vDir.xz / (0.02 + 0.98 * abs(vDir.y));

    // local cloud pos
    vec3 pos;
    pos.y = 0.0;
    pos.xz = NL_CLOUD2_SCALE * (vPos.xz + vec2(1.0, 0.5) * seed);
    pos += deltaP;

    deltaP /= -float(NL_CLOUD2_STEPS);

    // alpha, gradient
    vec2 d = vec2(0.0, 1.0);
    for (int i = 1; i <= NL_CLOUD2_STEPS; i++) {
        float m = cloudDf(pos, rain, seed);

        d.x += m;
        d.y = mix(d.y, pos.y, m);

        pos += deltaP;
    }

    d.x *= smoothstep(0.03, 0.1, d.x);
    d.x = d.x / ((float(NL_CLOUD2_STEPS) / NL_CLOUD2_DENSITY) + d.x);

    if (vPos.y > 0.0) { // view from bottom
        d.y = 1.0 - d.y;
    }

    d.y = 1.0 - 0.7 * d.y * d.y;

    vec4 col = vec4(0.6 * skyCol, d.x);
    col.rgb += (vec3(0.03, 0.05, 0.05) + 0.8 * fogCol) * d.y;
    col.rgb *= 1.0 - 0.5 * rain;

    return col;
}
#elif TYPEN == 4

// Hash function to generate pseudo-random values
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Simple noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

// Fractal Brownian Motion (FBM) with fewer octaves for optimization
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;

    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

float cloudDf(vec3 pos, float rain) {
    vec2 p0 = floor(pos.xz);
    vec2 u = smoothstep(0.999*NL_CLOUD2_SHAPE, 1.0, pos.xz-p0);
    vec2 t = vec2(0.1001+0.2*rain, 0.1+0.2*rain*rain);
    float n = mix(
        mix(fbm(p0 + t), fbm(p0+vec2(1.0,0.0) + t), u.x),
        mix(fbm(p0+vec2(0.0,1.0) + t), fbm(p0+vec2(1.0,1.0) + t), u.x),
        u.y
    );
    float b = 1.0 - 1.9*smoothstep(NL_CLOUD2_SHAPE, 2.0 - NL_CLOUD2_SHAPE, 2.0*abs(pos.y-0.5));
    return smoothstep(0.2, 1.0, n * b);
}

vec4 renderClouds(vec3 vDir, vec3 vPos, float rain, float time, vec3 fogCol, vec3 skyCol) {

  float height = 7.0*mix(NL_CLOUD2_THICKNESS, NL_CLOUD2_RAIN_THICKNESS, rain);

  // scaled ray offset
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = (NL_CLOUD2_SCALE*height)*vDir.xz/(0.02+0.98*abs(vDir.y));
  //deltaP.xyz = (NL_CLOUD2_SCALE*height)*vDir.xyz;
  //deltaP.y = abs(deltaP.y);

  // local cloud pos
  vec3 pos;
  pos.y = 0.0;
  pos.xz = NL_CLOUD2_SCALE*(vPos.xz + vec2(1.0,0.5)*(time*NL_CLOUD2_VELOCIY));
  pos += deltaP;

  deltaP /= -float(NL_CLOUD2_STEPS);

  // alpha, gradient
  vec2 d = vec2(0.0,1.0);
  for (int i=1; i<=NL_CLOUD2_STEPS; i++) {
    float m = cloudDf(pos, rain);

    d.x += m;
    d.y = mix(d.y, pos.y, m);

    //if (d.x == 0.0 && i > NL_CLOUD2_STEPS/2) {
    //        break;
    //} 

    pos += deltaP;
  }
  //d.x *= vDir.y*vDir.y; 
  d.x *= smoothstep(0.03, 0.1, d.x);
  d.x = d.x / ((float(NL_CLOUD2_STEPS)/NL_CLOUD2_DENSITY) + d.x);

  if (vPos.y > 0.0) { // view from bottom
    d.y = 1.0 - d.y;
  }

  d.y = 1.0 - 0.7*d.y*d.y;

  vec4 col = vec4(0.6*skyCol, d.x);
  col.rgb += (vec3(0.03,0.05,0.05) + 0.8*fogCol)*d.y;
  col.rgb *= 1.0 - 0.5*rain;

  return col;
}
#elif TYPEN == 5
#define SCALE 1.0
#define OCTAVES 3
#define ANIMATION_SPEED 0.2

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 st) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < OCTAVES; i++) {
        value += amplitude * noise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

float cloudDf(vec3 pos, float rain) {
    vec2 p0 = floor(pos.xz * SCALE);
    vec2 u = smoothstep(0.999*NL_CLOUD2_SHAPE, 1.0, pos.xz-p0);
    vec2 t = vec2(0.1001+0.2*rain, 0.1+0.2*rain*rain);
    float n = mix(
        mix(fbm(p0 + t), fbm(p0+vec2(1.0,0.0) + t), u.x),
        mix(fbm(p0+vec2(0.0,1.0) + t), fbm(p0+vec2(1.0,1.0) + t), u.x),
        u.y
    );
    float b = 1.0 - 1.9*smoothstep(NL_CLOUD2_SHAPE, 2.0 - NL_CLOUD2_SHAPE, 2.0*abs(pos.y-0.5));
    return smoothstep(0.2, 1.0, n * b);
}
vec4 renderClouds(vec3 vDir, vec3 vPos, float rain, float time, vec3 fogCol, vec3 skyCol) {

  float height = 7.0*mix(NL_CLOUD2_THICKNESS, NL_CLOUD2_RAIN_THICKNESS, rain);

  // scaled ray offset
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = (NL_CLOUD2_SCALE*height)*vDir.xz/(0.02+0.98*abs(vDir.y));
  //deltaP.xyz = (NL_CLOUD2_SCALE*height)*vDir.xyz;
  //deltaP.y = abs(deltaP.y);

  // local cloud pos
  vec3 pos;
  pos.y = 0.0;
  pos.xz = NL_CLOUD2_SCALE*(vPos.xz + vec2(1.0,0.5)*(time*NL_CLOUD2_VELOCIY));
  pos += deltaP;

  deltaP /= -float(NL_CLOUD2_STEPS);

  // alpha, gradient
  vec2 d = vec2(0.0,1.0);
  for (int i=1; i<=NL_CLOUD2_STEPS; i++) {
    float m = cloudDf(pos, rain);

    d.x += m;
    d.y = mix(d.y, pos.y, m);

    //if (d.x == 0.0 && i > NL_CLOUD2_STEPS/2) {
    //        break;
    //} 

    pos += deltaP;
  }
  //d.x *= vDir.y*vDir.y; 
  d.x *= smoothstep(0.03, 0.1, d.x);
  d.x = d.x / ((float(NL_CLOUD2_STEPS)/NL_CLOUD2_DENSITY) + d.x);

  if (vPos.y > 0.0) { // view from bottom
    d.y = 1.0 - d.y;
  }

  d.y = 1.0 - 0.7*d.y*d.y;

  vec4 col = vec4(0.6*skyCol, d.x);
  col.rgb += (vec3(0.03,0.05,0.05) + 0.8*fogCol)*d.y;
  col.rgb *= 1.0 - 0.5*rain;

  return col;
}
#elif TYPEN == 6
float cloudDf(vec3 pos, float rain) {
  vec2 p0 = floor(pos.xz);
  vec2 u = smoothstep(0.999*NL_CLOUD2_SHAPE, 1.0, pos.xz-p0);

  // rain transition
  vec2 t = vec2(0.1001+0.2*rain, 0.1+0.2*rain*rain);

  float n = mix(
    mix(randt(p0, t),randt(p0+vec2(1.0,0.0), t), u.x),
    mix(randt(p0+vec2(0.0,1.0), t),randt(p0+vec2(1.0,1.0), t), u.x),
    u.y
  );

  // round y
  float b = 1.0 - 1.9*smoothstep(NL_CLOUD2_SHAPE, 2.0 - NL_CLOUD2_SHAPE, 2.0*abs(pos.y-0.5));
  return smoothstep(0.2, 1.0, n * b);
}

vec4 renderClouds(vec3 vDir, vec3 vPos, float rain, float time, vec3 fogCol, vec3 skyCol) {

  float height = 7.0*mix(NL_CLOUD2_THICKNESS, NL_CLOUD2_RAIN_THICKNESS, rain);

  // scaled ray offset
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = (NL_CLOUD2_SCALE*height)*vDir.xz/(0.02+0.98*abs(vDir.y));
  //deltaP.xyz = (NL_CLOUD2_SCALE*height)*vDir.xyz;
  //deltaP.y = abs(deltaP.y);

  // local cloud pos
  vec3 pos;
  pos.y = 0.0;
  pos.xz = NL_CLOUD2_SCALE*(vPos.xz + vec2(1.0,0.5)*(time*NL_CLOUD2_VELOCIY));
  pos += deltaP;

  deltaP /= -float(NL_CLOUD2_STEPS);

  // alpha, gradient
  vec2 d = vec2(0.0,1.0);
  for (int i=1; i<=NL_CLOUD2_STEPS; i++) {
    float m = cloudDf(pos, rain);

    d.x += m;
    d.y = mix(d.y, pos.y, m);

    //if (d.x == 0.0 && i > NL_CLOUD2_STEPS/2) {
    //        break;
    //} 

    pos += deltaP;
  }
  //d.x *= vDir.y*vDir.y; 
  d.x *= smoothstep(0.03, 0.1, d.x);
  d.x = d.x / ((float(NL_CLOUD2_STEPS)/NL_CLOUD2_DENSITY) + d.x);

  if (vPos.y > 0.0) { // view from bottom
    d.y = 1.0 - d.y;
  }

  d.y = 1.0 - 0.7*d.y*d.y;

  vec4 col = vec4(0.6*skyCol, d.x);
  col.rgb += (vec3(0.03,0.05,0.05) + 0.8*fogCol)*d.y;
  col.rgb *= 1.0 - 0.5*rain;

  return col;
}
#endif
// aurora is rendered on clouds layer
#ifdef NL_AURORA
vec4 renderAurora(vec3 p, float t, float rain, vec3 FOG_COLOR) {
  t *= NL_AURORA_VELOCITY;
  p.xz *= NL_AURORA_SCALE;
  p.xz += 0.05*sin(p.x*4.0 + 20.0*t);

  float d0 = sin(p.x*0.1 + t + sin(p.z*0.2));
  float d1 = sin(p.z*0.1 - t + sin(p.x*0.2));
  float d2 = sin(p.z*0.1 + 1.0*sin(d0 + d1*2.0) + d1*2.0 + d0*1.0);
  d0 *= d0; d1 *= d1; d2 *= d2;
  d2 = d0/(1.0 + d2/NL_AURORA_WIDTH);

  float mask = (1.0-0.8*rain)*max(1.0 - 3.0*max(FOG_COLOR.b, FOG_COLOR.g), 0.0);
  return vec4(NL_AURORA*mix(NL_AURORA_COL1,NL_AURORA_COL2,d1),1.0)*d2*mask;
}
#endif

#endif