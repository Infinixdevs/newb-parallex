vec4 a_color0     : COLOR0;
vec3 a_position   : POSITION;
vec2 a_texcoord0  : TEXCOORD0;
vec2 a_texcoord1  : TEXCOORD1;

vec4 i_data0      : TEXCOORD8;
vec4 i_data1      : TEXCOORD7;
vec4 i_data2      : TEXCOORD6;
vec4 i_data3      : TEXCOORD5;

vec4 v_color0     : COLOR0;
vec4 v_color1     : COLOR1;
vec4 v_fog        : COLOR2;
vec4 v_refl       : COLOR3;
vec3 v_underwaterRainTime       : COLOR4;

centroid vec2 v_texcoord0  : TEXCOORD0;
vec2 v_lightmapUV : TEXCOORD1;
vec3 v_position   : TEXCOORD2;
vec4 v_extra      : TEXCOORD3;
vec3          v_wpos       : POSITION2; // World Pos
vec4          v_color2     : TEXCOORD9;
vec3          v_position   : TEXCOORD10;
vec3 relPos : RELATIVE_POSITION;
vec3 fragPos : FRAGMENT_POSITION;
float Time : FRAME_TIME_COUNTER;
float waterFlag : WATER_FLAG;
vec2 fogControl : FOG_CONTROL;
float v_rainDrops : RAIN_DROP;
//vec3 v_rainDrops : TEXCOORD11;