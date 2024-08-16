$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_wpos, relPos, fragPos, Time, waterFlag, fogControl, v_underwaterRainTime, v_color2, v_position, v_rainDrops

#include <bgfx_shader.sh>
#include <newb/main.sh>

SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

uniform vec4 ViewPositionAndTime;
uniform vec4 FogColor;
int getBlockID(const vec4 texCol) {
    bool iron     = 0.99 <= texCol.a && texCol.a < 1.00;
    bool gold     = 0.98 <= texCol.a && texCol.a < 0.99;
    bool copper   = 0.97 <= texCol.a && texCol.a < 0.98;
    bool other    = 0.96 <= texCol.a && texCol.a < 0.97;

    return iron ? 0 : gold ? 1 : copper ? 2 : other ? 3 : 4;
}
float computeAmbientOcclusion(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 fragPos) {
    // Use a combination of dot products to consider light direction and surface normals
    float occlusion = 0.0;

    float normalAO = max(dot(normal, lightDir), 0.0);
    
    float viewAO = max(dot(normal, viewDir), 0.0);

    // Position-based occlusion using fragment position (consider depth)
    float posAO = clamp(length(fragPos) / 50.0, 0.0, 1.0);

    // Combine these effects
    occlusion = normalAO * 0.5 + viewAO * 0.3 + posAO * 0.2;

    // Ensure smooth blending
    occlusion = pow(occlusion, 0.5);

    return occlusion;
}

// https://github.com/bWFuanVzYWth/OriginShader
float fogTime(float fogColorG){
    return clamp(((349.305545 * fogColorG - 159.858192) * fogColorG + 30.557216) * fogColorG - 1.628452, -1.0, 1.0);
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
    float aoFactor = computeAmbientOcclusion(normal, lightDir, viewDir, fragPos);

    // Apply ambient occlusion to the diffuse color
    diffuse.rgb *= aoFactor;
#elif AO ==2
// Normal calculation (using provided data or derive from texture)
vec3 normal = fnormal;

// Light direction (dynamic)
vec3 lightDir = normalize(lightPos);

// Compute the enhanced ambient occlusion
float aoFactor = computeAmbientOcclusion(normal, lightDir, viewDir, v_position);

// Apply ambient occlusion to the diffuse color
diffuse.rgb *= aoFactor;
#endif
vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

    diffuse.rgb *= diffuse.rgb;

    vec3 lightTint = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
    lightTint = mix(lightTint.bbb, lightTint * lightTint, 0.35 + 0.65 * v_lightmapUV.y * v_lightmapUV.y * v_lightmapUV.y);

    color.rgb *= lightTint;
 #ifdef METALLIC
   // Determine block type
    int blockID = getBlockID(diffuse);
    bool isMetallic = (blockID == 0 || blockID == 1 || blockID == 2); // iron, gold, copper

    // Fake metallic effect start
    vec3 zenithCol;
    vec3 horizonCol;
    vec3 horizonEdgeCol;
    
bool underWater = v_underwaterRainTime.x > 0.5;
    float rainFactor = v_underwaterRainTime.y;
    
    if (underWater) {
        vec3 fogcol = getUnderwaterCol(v_fog.rgb);
        zenithCol = fogcol;
        horizonCol = fogcol;
        horizonEdgeCol = fogcol;
    } else {
        vec3 fs = getSkyFactors(v_fog.rgb);
        zenithCol = getZenithCol(rainFactor, v_fog.rgb, fs);
        horizonCol = getHorizonCol(rainFactor, v_fog.rgb, fs);
        horizonEdgeCol = getHorizonEdgeCol(horizonCol, rainFactor, v_fog.rgb);
    }

    if (isMetallic) {
        // Combine the zenith and horizon colors for a metallic shine
        vec3 metallicShine = mix(zenithCol, horizonCol, 0.5) * 0.6;
        diffuse.rgb += metallicShine * 0.5;
    }
    
    #endif
    
    //AO component
    vec3 ncol_0 = normalize(v_color0.rgb);
    if(abs(ncol_0.r - ncol_0.g) > 0.001 || abs(ncol_0.g - ncol_0.b) > 0.001) {
        diffuse = vec4(diffuse.rgb * mix(ncol_0.rgb, v_color0.rgb, 0.45), v_color0.a);
    }
    

    
    #ifdef TRANSPARENT
        if (v_extra.b > 0.9) {
            diffuse.rgb = vec3_splat(1.0 - NL_WATER_TEX_OPACITY * (1.0 - diffuse.b * 1.8));
            diffuse.a = color.a;
        }
    #else
        diffuse.a = 1.0;
    #endif

    float vanillaAO = 0.0;
    #ifndef SEASONS
        vanillaAO = 1.0 - (v_color0.g * 2.0 - (v_color0.r < v_color0.b ? v_color0.r : v_color0.b));
    #endif
    //diffuse.rgb = v_rainDrops;

    texCol.rgb = diffuse.rgb;
    diffuse.rgb *= color.rgb;
    diffuse.rgb += glow;


    if (v_extra.b > 0.9) {
        diffuse.rgb += v_refl.rgb * v_refl.a;
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

