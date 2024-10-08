#ifndef TONEMAP_H
#define TONEMAP_H


vec3 colorCorrection(vec3 col) {
    #ifdef NL_EXPOSURE
        col *= NL_EXPOSURE;
    #endif

    #if NL_TONEMAP_TYPE == 10
        // Unreal Engine tonemap
        col = col / (col + 0.155) * 1.019;
    #elif NL_TONEMAP_TYPE == 3
        // Extended Reinhard tonemap
        const float whiteScale = 0.063;
        col = col * (1.0 + col * whiteScale) / (1.0 + col);
    #elif NL_TONEMAP_TYPE == 4
        // ACES tonemap
        mat3 m1 = mat3(
            0.59719, 0.07600, 0.02840,
            0.35458, 0.90834, 0.13383,
            0.04823, 0.01566, 0.83777
        );
        mat3 m2 = mat3(
            1.60475, -0.10208, -0.00327,
            -0.53108,  1.10813, -0.07276,
            -0.07367, -0.00605,  1.07602
        );
        vec3 v = m1 * col;
        vec3 a = v * (v + 0.0245786) - 0.000090537;
        vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
        col = pow(clamp(m2 * (a / b), 0.0, 1.0), vec3(1.0 / 2.2));
    #elif NL_TONEMAP_TYPE == 2
        // Simple Reinhard tonemap
        col = col / (1.0 + col);
    #elif NL_TONEMAP_TYPE == 1
        // Exponential tonemap
        col = 1.0 - exp(-col * 0.8);
    #elif NL_TONEMAP_TYPE == 5
        // Filmic tonemap
        col = (col * (2.51 * col + 0.03)) / (col * (2.43 * col + 0.59) + 0.14);
    #elif NL_TONEMAP_TYPE == 6
        // Hejl 2015 tonemap
        col = max(vec3(0.0), (col * (col + 0.0245786 - 0.000090537)) / (col * (0.983729 * col + 0.4329510 + 0.238081)));
    #elif NL_TONEMAP_TYPE == 7
        // Hable tonemap
        col = (col * (col * 0.6 + 0.5)) / (col * (col * 0.3 + 0.6) + 0.1);
    #elif NL_TONEMAP_TYPE == 8
        // Uncharted 2 tonemap
        col = (col * (col * 0.426 + 0.55)) / (col * (col * 0.3 + 0.45) + 0.05);
    #elif NL_TONEMAP_TYPE == 9
        // Reinhard Extended (modified) tonemap
        col = col * (1.0 + col * 0.2) / (1.0 + col);
    #elif NL_TONEMAP_TYPE == 11
        // Drago tonemap
        const float Ld_max = 100.0;
        const float Lwa = 0.5;
        const float L = max(col.r, max(col.g, col.b));
        col = col * (1.0 + col / (Ld_max * Lwa * Lwa)) / (1.0 + col);
    #elif NL_TONEMAP_TYPE == 12
        // Lottes tonemap
        float A = 1.6;
        float B = 0.42;
        float C = 1.33;
        float D = 0.3;
        float E = 0.11;
        float F = 0.5;
        col = ((col * (A * col + B * F)) / (col * (A * col + D * E)) - F) / (C * col + D * E - F);
    #elif NL_TONEMAP_TYPE == 13
        // Ward tonemap
        float W = 11.2;
        col = col / (1.0 + col / W);
    #elif NL_TONEMAP_TYPE == 14
        // Exponential2 tonemap
        col = 1.0 - exp(-col * 0.5);
    #elif NL_TONEMAP_TYPE == 15
        // Uchimura tonemap
        float P = 1.0;
        float Q = 1.0;
        float A = 1.0;
        float B = 1.0;
        float C = 1.0;
        float D = 1.0;
        float E = 1.0;
        col = col * (P * col + Q) / (A * col + B) + (C * col + D) / (E * col + 1.0);
    #elif NL_TONEMAP_TYPE == 16
        // Rational tonemap
        col = col / (1.0 + col / 0.5);
    #elif NL_TONEMAP_TYPE == 17
        // Reinhard (advanced) tonemap
        col = col * (1.0 + col / (1.0 + col / 0.5)) / (1.0 + col);
    #elif NL_TONEMAP_TYPE == 18
        // Schlick tonemap
        col = col / (0.5 + col * (1.0 - 0.5));
    #elif NL_TONEMAP_TYPE == 19
        // Gamma tonemap
        col = pow(col, vec3(1.0 / 2.2));
    #endif

    // Gamma correction + contrast
    col = pow(col, vec3_splat(NL_CONSTRAST));

    #ifdef NL_SATURATION
        col = mix(vec3_splat(dot(col, vec3(0.21, 0.71, 0.08))), col, NL_SATURATION);
    #endif

    #ifdef NL_TINT
        col *= NL_TINT;
    #endif

    return col;
}

// Inverse function used in fogcolor for nether
vec3 colorCorrectionInv(vec3 col) {
    #ifdef NL_TINT
        col /= NL_TINT;
    #endif

    float ws = 0.7966;
    col = pow(col, vec3_splat(1.0 / NL_CONSTRAST));
    col = col * (ws + col) / (ws + col * (1.0 - ws));

    #ifdef NL_EXPOSURE
        col /= NL_EXPOSURE;
    #endif

    return col;
}

#endif
