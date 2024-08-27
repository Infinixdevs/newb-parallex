$input a_color0, a_position, a_texcoord0, a_texcoord1
#ifdef INSTANCING
  $input i_data0, i_data1, i_data2, i_data3
#endif
$output v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_wpos, relPos, fragPos, Time, waterFlag, fogControl, v_underwaterRainTime, v_color2, v_position, v_rainDrops, sPos

#include <bgfx_shader.sh>
#include <newb/main.sh>

uniform vec4 RenderChunkFogAlpha;
uniform vec4 FogAndDistanceControl;
uniform vec4 ViewPositionAndTime;
uniform vec4 FogColor;

void main() {
  #ifdef INSTANCING
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
  #else
    mat4 model = u_model[0];
  #endif
    vec3 worldPoss = mul(model, vec4(a_position, 1.0)).xyz;
  vec3 worldPos = mul(model, vec4(a_position, 1.0)).xyz;
    relPos += vec3(0.5, 0.5, 0.5);
        vec3 viewDr = normalize(relPos - ViewPositionAndTime.xyz);
  #ifdef RENDER_AS_BILLBOARDS
    worldPos += vec3(0.5,0.5,0.5);

    vec3 modelCamPos = ViewPositionAndTime.xyz - worldPos;
    float camDis = length(modelCamPos);
    vec3 viewDir = modelCamPos / camDis;

    vec3 boardPlane = normalize(vec3(-viewDir.z, 0.0, viewDir.x));
    worldPos -= (((viewDir.zxy * boardPlane.yzx) - (viewDir.yzx * boardPlane.zxy)) *
                 (a_color0.z - 0.5)) +
                 (boardPlane * (a_color0.x - 0.5));
    vec4 color = vec4(1.0,1.0,1.0,1.0);
  #else
    vec3 modelCamPos = (ViewPositionAndTime.xyz - worldPos);
    float camDis = length(modelCamPos);
    vec3 viewDir = modelCamPos / camDis;

    vec4 color = a_color0;
  #endif

  float relativeDist = camDis / FogAndDistanceControl.z;

  vec3 cPos = a_position.xyz;
  vec3 bPos = fract(cPos);
  vec3 tiledCpos = fract(cPos*0.0625);

  vec2 uv1 = a_texcoord1;
  vec2 lit = uv1*uv1;

  bool isColored = color.r != color.g || color.r != color.b;
  float shade = isColored ? color.g*1.5 : color.g;

  // tree leaves detection
  #ifdef ALPHA_TEST
    bool isTree = (isColored && (bPos.x+bPos.y+bPos.z < 0.001)) || color.a == 0.0;
  #else
    bool isTree = false;
  #endif

  // environment detections
  bool end = detectEnd(FogColor.rgb, FogAndDistanceControl.xy);
  bool nether = detectNether(FogColor.rgb, FogAndDistanceControl.xy);
  bool underWater = detectUnderwater(FogColor.rgb, FogAndDistanceControl.xy);
  float rainFactor = detectRain(FogAndDistanceControl.xyz);

  // sky colors
  vec3 zenithCol;
  vec3 horizonCol;
  vec3 horizonEdgeCol;
  if (underWater) {
    vec3 fogcol = getUnderwaterCol(FogColor.rgb);
    zenithCol = fogcol;
    horizonCol = fogcol;
    horizonEdgeCol = fogcol;
  } else if (end) {
    zenithCol = getEndZenithCol();
    horizonCol = getEndHorizonCol();
    horizonEdgeCol = horizonCol;
  } else {
    vec3 fs = getSkyFactors(FogColor.rgb);
    zenithCol = getZenithCol(rainFactor, FogColor.rgb, fs);
    horizonCol = getHorizonCol(rainFactor, FogColor.rgb, fs);
    horizonEdgeCol = getHorizonEdgeCol(horizonCol, rainFactor, FogColor.rgb);
  }

  // time
  highp float t = ViewPositionAndTime.w;

  // convert color space to linear-space
  #ifdef SEASONS
    isTree = true;

    // season tree leaves are colored in fragment
    color.w *= color.w;
    color = vec4(color.www, 1.0);
  #else
    if (isColored) {
      color.rgb *= color.rgb*1.2;
    }
  #endif

  vec3 torchColor; // modified by nl_lighting
  vec3 light = nlLighting(
    worldPos, torchColor, a_color0.rgb, FogColor.rgb, rainFactor,uv1, lit, isTree, horizonCol, zenithCol, shade, end, nether, underWater, t
  );

  #if defined(ALPHA_TEST) && (defined(NL_PLANTS_WAVE) || defined(NL_LANTERN_WAVE))
    nlWave(worldPos, light, rainFactor, uv1, lit, a_texcoord0, bPos, a_color0, cPos, tiledCpos, t, isColored, camDis, isTree);
  #endif

  #ifdef NL_CHUNK_LOAD_ANIM
    // slide in anim
    worldPos.y -= NL_CHUNK_LOAD_ANIM*pow(RenderChunkFogAlpha.x,3.0);
  #endif

  // loading chunks
  relativeDist += RenderChunkFogAlpha.x;

  vec4 fogColor;
  fogColor.rgb = nlRenderSky(horizonEdgeCol, horizonCol, zenithCol, viewDir, FogColor.rgb, t, rainFactor, end, underWater, nether);
  fogColor.a = nlRenderFogFade(relativeDist, FogColor.rgb, FogAndDistanceControl.xy);
  #ifdef NL_GODRAY 
    fogColor.a = mix(fogColor.a, 1.0, NL_GODRAY*nlRenderGodRayIntensity(cPos, worldPos, t, uv1, relativeDist, FogColor.rgb));
  #endif

fogControl = FogAndDistanceControl.xy;
  if (nether) {
    // blend fog with void color
    fogColor.rgb = colorCorrectionInv(FogColor.rgb);
    fogColor.rgb = mix(fogColor.rgb, vec3(0.8,0.2,0.12)*1.5, lit.x*(1.67-fogColor.a*1.67));
  }

  vec4 refl = vec4(0.0,0.0,0.0,0.0);
  vec4 pos;

  #if !defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY)
  
  fragPos = a_position;
Time = ViewPositionAndTime.w; 

waterFlag = 0.0;
    #ifdef TRANSPARENT
      if (a_color0.a < 0.95) {
        color.a += (0.5-0.5*color.a)*clamp((camDis/FogAndDistanceControl.w),0.0,1.0);
      };

      float water;
      if (a_color0.b > 0.3 && a_color0.a < 0.95) {
        water = 1.0;
        refl = nlWater(
          worldPos, color, a_color0, viewDir, light, cPos, tiledCpos, bPos.y, FogColor.rgb, horizonCol, horizonEdgeCol, zenithCol, lit, t, camDis, rainFactor, torchColor, end, nether, underWater
        );
        pos = mul(u_viewProj, vec4(worldPos, 1.0));
      } else {
        water = 0.0;
        pos = mul(u_viewProj, vec4(worldPos, 1.0));
        refl = nlRefl(
          color, fogColor, lit, uv1, tiledCpos, camDis, worldPos, viewDir, torchColor, horizonEdgeCol, horizonCol, zenithCol, FogColor.rgb, rainFactor, FogAndDistanceControl.z, t, pos.xyz, underWater, end, nether
        );
      }
    #else
      float water = 0.0;
      pos = mul(u_viewProj, vec4(worldPos, 1.0));
      refl = nlRefl(
        color, fogColor, lit, uv1, tiledCpos, camDis, worldPos, viewDir, torchColor, horizonEdgeCol, horizonCol, zenithCol, FogColor.rgb, rainFactor, FogAndDistanceControl.z, t, pos.xyz, underWater, end, nether
      );
      waterFlag = 1.0;
    #endif

    if (underWater) {
      nlUnderwaterLighting(light, pos.xyz, lit, uv1, tiledCpos, cPos, t, horizonEdgeCol);
    }
  #else
    float water = 0.0;
    pos = mul(u_viewProj, vec4(worldPos, 1.0));
  #endif

  color.rgb *= light;
// Lens flare effect for terrain
float flareIntensity = pow(max(dot(normalize(viewDir), normalize(worldPos)), 0.0), 2.0);
v_color2 = vec4(a_texcoord1 + flareIntensity * vec2(0.1, 0.1), 0.0, 0.0);

    v_position = a_position.xyz;
  #ifdef NL_GLOW_SHIMMER
    float shimmer = nlGlowShimmer(cPos, t);
  #else
    float shimmer = 0.0;
  #endif
    v_underwaterRainTime.x = float(detectUnderwater(FogColor.rgb, FogAndDistanceControl.xy));
    v_underwaterRainTime.y = detectRain(FogAndDistanceControl.xyz);
    v_underwaterRainTime.z = ViewPositionAndTime.w;
    
    // Rain screen drops effect
float rainDropIntensity = rainFactor * 0.5; // Adjust intensity as needed
vec2 rainDropPosition = a_texcoord0.xy * 2.0 - 1.0; // Map to screen space

// Simulate the movement of rain drops over time
vec2 rainMovement = rainDropPosition + vec2(0.0, Time * 0.2);

// Create a simple drop pattern using a sine function
float dropEffect = max(0.0, sin(10.0 * rainMovement.x) * sin(10.0 * rainMovement.y));

// Modulate the effect by rain intensity
v_rainDrops = dropEffect * rainDropIntensity;

vec3 newpos = a_position;

    // make sky curved
    newpos.y -= 0.4*a_color0.r*a_color0.r;
  vec3 sposv = newpos.xyz;
  sposv.y += 0.148;
  
    v_underwaterRainTime.x = float(detectUnderwater(FogColor.rgb, FogAndDistanceControl.xy));
    v_underwaterRainTime.y = detectRain(FogAndDistanceControl.xyz);
    v_underwaterRainTime.z = ViewPositionAndTime.w;
    

  v_extra = vec4(shade, worldPos.y, water, shimmer);
  v_refl = refl;
  v_texcoord0 = a_texcoord0;
  v_lightmapUV = a_texcoord1;
  v_color0 = color;
  v_color1 = a_color0;
  v_fog = fogColor;
  sPos = sposv;
           v_wpos = worldPoss;
  gl_Position = pos;
}
