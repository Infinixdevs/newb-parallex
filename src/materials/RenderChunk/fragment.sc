$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_wpos, relPos, fragPos, Time, waterFlag, fogControl, v_underwaterRainTime, v_color2, v_position, v_rainDrops, sPos

#include <bgfx_shader.sh>
#include <newb/main.sh>

SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

uniform vec4 ViewPositionAndTime;
uniform vec4 FogColor;
#define DAY_CLOUD_COL vec3(1.0, 1.0, 1.0)
#define DAY_CLOUD_SHADE_COL vec3(0.67, 0.72, 0.82)
#define CLOUD_VARIATION 0.2
#define SKY_COL vec3(0.52, 0.71, 0.91)
#define HORIZON_COL  vec3(0.1, 0.9, 1.0)
#define ZENITH_COL vec3(0.231, 0.353, 0.722)

int getBlockID(const vec4 texCol) {
    bool iron     = 0.99 <= texCol.a && texCol.a < 1.00;
    bool gold     = 0.98 <= texCol.a && texCol.a < 0.99;
    bool copper   = 0.97 <= texCol.a && texCol.a < 0.98;
    bool other    = 0.96 <= texCol.a && texCol.a < 0.97;

    return iron ? 0 : gold ? 1 : copper ? 2 : other ? 3 : 4;
}
float computeAmbientOcclusion(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 fragPos, float darkness) {
    // Use a combination of dot products to consider light direction and surface normals
    float occlusion = 0.0;
    float normalAO = max(dot(normal, lightDir), 0.0);
    float viewAO = max(dot(normal, viewDir), 0.0);
    // Position-based occlusion using fragment position (consider depth)
    float posAO = clamp(length(fragPos) / 50.0, 0.0, 1.0);
    // Combine these effects
    occlusion = normalAO * 0.5 + viewAO * 0.3 + posAO * 0.2;
    // Apply darkness factor
    occlusion *= darkness;
    // Ensure smooth blending
    occlusion = pow(occlusion, 0.28);
    return occlusion;
}

// https://github.com/bWFuanVzYWth/OriginShader
float fogTime(float fogColorG){
    return clamp(((349.305545 * fogColorG - 159.858192) * fogColorG + 30.557216) * fogColorG - 1.628452, -1.0, 1.0);
}

// Falling Stars code By i11212 : https://www.shadertoy.com/view/mdVXDm

highp float hashS(
        highp vec2 x){
return fract(sin(dot(
        x,vec2(11,57)))*4e3);
        }

highp float star(
        highp vec2 x, float time){
x = mul(x, mtxFromCols(vec2(cos(0.0), sin(0.0)), vec2(sin(0.0), -cos(0.5))));
x.y += time*22.0;
highp float shape = (0.9-length(
        fract(x-vec2(0,0.5))-0.5));
x *= vec2(1,0.1);
highp vec2 fr = fract(x);
highp float random = step(hashS(floor(x)),0.01),
                        tall = (1.0-(abs(fr.x-0.5)+fr.y*0.5))*random;
return clamp(clamp((shape-random)*step(hashS(
        floor(x+vec2(0,0.05))),.01),0.0,1.0)+tall,0.0,1.0);
        }
        
float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}

float render2DClouds(const vec2 poss, const float time) {
    vec2 p = poss;
    p.x += time * 0.9;
    float body = hash12(floor(p));
    body = (body > 0.8) ? 1.0 : 0.0;
    return body;
}

vec2 renderThickClouds(const vec3 poss, const float time) {
    const int steps = 12;
    const float stepSize = 0.008;
    float clouds = 0.0;
    float cHeight = 1.3;
    float drawSpace = smoothstep(0.0, 1.0, length(poss.xz / (poss.y * float(12))));
    if (drawSpace < 1.0 && !bool(step(poss.y, 0.0))) {
        for (int i = 0; i < steps; i++) {
            float height = 1.0 + float(i) * stepSize;
            vec2 cloudPos = poss.xz / poss.y * height;
            cloudPos *= 2.5;
            clouds += render2DClouds(cloudPos, time);
            if (i == 0) {
                cHeight = render2DClouds(cloudPos, time);
            }
        }
        clouds = clouds > 0.0 ? 1.0 : 0.0;
        clouds = mix(clouds, 0.0, drawSpace);
    }
    return vec2(clouds, cHeight);
}

// https://github.com/origin0110/OriginShader
float getTime(const vec4 fogCol) {
	return fogCol.g > 0.213101 ? 1.0 : 
		dot(vec4(fogCol.g * fogCol.g * fogCol.g, fogCol.g * fogCol.g, fogCol.g, 1.0), 
			vec4(349.305545, -159.858192, 30.557216, -1.628452));
}

void main() {
    vec4 diffuse;
    vec4 color;
    vec4 texCol = vec4(0.0, 0.0, 0.0, 0.0);

    #if defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY)
        diffuse = vec4(1.0, 1.0, 1.0, 1.0);
        color = vec4(1.0, 1.0, 1.0, 1.0);
    #else
        diffuse = texture2D(s_MatTexture, v_texcoord0);

        #ifdef ALPHA_TEST
            if (diffuse.a < 0.6) {
                discard;
            }
        #endif

        #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
            diffuse.rgb *= mix(vec3(1.0, 1.0, 1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
        #endif

        color = v_color0;
    #endif

        vec3 pos = normalize(relPos);
    vec3 viewDir = -pos;

    vec3 fnormal = normalize(cross(dFdx(v_position), dFdy(v_position)));
    float sunAngle = fogTime(FogColor.g);
    vec3 sunPos = normalize(vec3(cos(sunAngle), sin(sunAngle), 0.0));
    
        vec3 lightPos = sunPos.y > 0.0 ? sunPos : -sunPos;
        #if AO ==1
     vec3 normal = normalize(v_color0.rgb * 2.0 - 1.0);

    // Light direction (can be a uniform or derived from some context)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));

    // Compute the enhanced ambient occlusion
float aoFactor = computeAmbientOcclusion(normal, lightDir, viewDir, fragPos, AO_DARKNESS);

    // Apply ambient occlusion to the diffuse color
    diffuse.rgb *= aoFactor;
#elif AO ==2
// Normal calculation (using provided data or derive from texture)
vec3 normal = fnormal;

// Light direction (dynamic)
vec3 lightDir = normalize(lightPos);

// Compute the enhanced ambient occlusion
float aoFactor = computeAmbientOcclusion(normal, lightDir, viewDir, v_position, AO_DARKNESS);

// Apply ambient occlusion to the diffuse color
diffuse.rgb *= aoFactor;
#endif
vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

    diffuse.rgb *= diffuse.rgb;

    vec3 lightTint = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
    lightTint = mix(lightTint.bbb, lightTint * lightTint, 0.35 + 0.65 * v_lightmapUV.y * v_lightmapUV.y * v_lightmapUV.y);

    color.rgb *= lightTint;

    
    //AO component
    vec3 ncol_0 = normalize(v_color0.rgb);
    if(abs(ncol_0.r - ncol_0.g) > 0.001 || abs(ncol_0.g - ncol_0.b) > 0.001) {
        diffuse = vec4(diffuse.rgb * mix(ncol_0.rgb, v_color0.rgb, 0.45), v_color0.a);
    }
    
      float rainFactor = v_underwaterRainTime.y;
  
     float mask = (1.0-1.0*rainFactor)*max(1.0 - 3.0*max(v_fog.b, v_fog.g), 0.0);
    #ifdef TRANSPARENT
        if (v_extra.b > 0.9) {
            diffuse.rgb = vec3_splat(1.0 - NL_WATER_TEX_OPACITY * (1.0 - diffuse.b * 1.8));
            diffuse.a = color.a;
        }
    #else
        diffuse.a = 1.0;
    #endif
   bool underWater = v_underwaterRainTime.x > 0.5;
    float vanillaAO = 0.0;
    //diffuse.rgb = v_rainDrops;
float timee = getTime(v_fog);
    texCol.rgb = diffuse.rgb;
    diffuse.rgb *= color.rgb;
    diffuse.rgb += glow;
 vec3 stars = pow(vec3_splat(star(sPos.zx*2.0, v_underwaterRainTime.z))*1.0, vec3(16,7,5))*mask;
 
  vec2 uv = v_texcoord0;
    vec3 poss = v_wpos;
    vec2 clouds = renderThickClouds(poss, Time);
    float variation = CLOUD_VARIATION * sin((uv.x + uv.x) * 0.1 + timee * 1.01);
    float tVariation = step(uv.x, 0.5) * step(uv.y, 0.5) * CLOUD_VARIATION;
    vec3 cloudColor = mix(DAY_CLOUD_SHADE_COL, DAY_CLOUD_COL, clouds.x + variation + tVariation);
    vec3 skyColor = mix(ZENITH_COL, HORIZON_COL, uv.x);
   /* vec4 cloudsrefl = vec4(mix(skyColor, cloudColor, clouds.x), 1.0);*/

    if (v_extra.b > 0.9) {
        diffuse.rgb += v_refl.rgb * v_refl.a;
        #ifdef STAR_REFL
        diffuse.rgb += stars;
        #endif
        diffuse.rgb += cloudColor;
    } else if (v_refl.a > 0.0) {
        // Reflective effect - only on xz plane
        float dy = abs(dFdy(v_extra.g));
        if (dy < 0.0002) {
            float mask = v_refl.a * (clamp(v_extra.r * 10.0, 8.2, 8.8) - 7.8);
            diffuse.rgb *= 1.0 - 0.6 * mask;
            diffuse.rgb += v_refl.rgb * mask;
        }
    }

    diffuse.rgb = mix(diffuse.rgb, v_fog.rgb, v_fog.a);
    diffuse.rgb = colorCorrection(diffuse.rgb);

    gl_FragColor = diffuse;
}

