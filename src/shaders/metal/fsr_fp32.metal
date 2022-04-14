// This is the fsr_pass.glsl shader, combined with the "ffx_a.h" and "ffx_fsr1.h" header files from
// FSR, and then transpiled into MSL. Therefore, this code is still licensed under MIT to AMD.
#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

constexpr sampler textureSampler(mag_filter::linear,
                                 min_filter::linear);

struct constantsBuffer
{
    uint4 const0;
    uint4 const1;
    uint4 const2;
    uint4 const3;
    uint4 Sample;
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(64u, 1u, 1u);

struct spvDescriptorSetBuffer0
{
    texture2d<float> InputTexture [[id(0)]];
    constant constantsBuffer* v_1315 [[id(1)]];
    texture2d<float, access::write> OutputTexture [[id(2)]];
};

static inline __attribute__((always_inline))
uint ABfe(thread const uint& src, thread const uint& off, thread const uint& bits)
{
    return extract_bits(src, uint(int(off)), uint(int(bits)));
}

static inline __attribute__((always_inline))
uint ABfiM(thread const uint& src, thread const uint& ins, thread const uint& bits)
{
    return insert_bits(src, ins, uint(0), uint(int(bits)));
}

static inline __attribute__((always_inline))
uint2 ARmp8x8(thread const uint& a)
{
    uint param = a;
    uint param_1 = 1u;
    uint param_2 = 3u;
    uint param_3 = a;
    uint param_4 = 3u;
    uint param_5 = 3u;
    uint param_6 = ABfe(param_3, param_4, param_5);
    uint param_7 = a;
    uint param_8 = 1u;
    return uint2(ABfe(param, param_1, param_2), ABfiM(param_6, param_7, param_8));
}

static inline __attribute__((always_inline))
float4 FsrEasuRF(thread const float2& p, texture2d<float> InputTexture, sampler InputSampler)
{
    float4 res = InputTexture.gather(InputSampler, p, int2(0), component::x);
    return res;
}

static inline __attribute__((always_inline))
float4 FsrEasuGF(thread const float2& p, texture2d<float> InputTexture, sampler InputSampler)
{
    float4 res = InputTexture.gather(InputSampler, p, int2(0), component::y);
    return res;
}

static inline __attribute__((always_inline))
float4 FsrEasuBF(thread const float2& p, texture2d<float> InputTexture, sampler InputSampler)
{
    float4 res = InputTexture.gather(InputSampler, p, int2(0), component::z);
    return res;
}

static inline __attribute__((always_inline))
float4 AF4_x(thread const float& a)
{
    return float4(a, a, a, a);
}

static inline __attribute__((always_inline))
float2 AF2_x(thread const float& a)
{
    return float2(a, a);
}

static inline __attribute__((always_inline))
float AF1_x(thread const float& a)
{
    return a;
}

static inline __attribute__((always_inline))
uint AU1_x(thread const uint& a)
{
    return a;
}

static inline __attribute__((always_inline))
float APrxLoRcpF1(thread const float& a)
{
    uint param = 2129690299u;
    return as_type<float>(AU1_x(param) - as_type<uint>(a));
}

static inline __attribute__((always_inline))
float ASatF1(thread const float& x)
{
    float param = 0.0;
    float param_1 = 1.0;
    return fast::clamp(x, AF1_x(param), AF1_x(param_1));
}

static inline __attribute__((always_inline))
void FsrEasuSetF(thread float2& dir, thread float& len, thread const float2& pp, thread const bool& biS, thread const bool& biT, thread const bool& biU, thread const bool& biV, thread const float& lA, thread const float& lB, thread const float& lC, thread const float& lD, thread const float& lE)
{
    float param = 0.0;
    float w = AF1_x(param);
    if (biS)
    {
        float param_1 = 1.0;
        float param_2 = 1.0;
        w = (AF1_x(param_1) - pp.x) * (AF1_x(param_2) - pp.y);
    }
    if (biT)
    {
        float param_3 = 1.0;
        w = pp.x * (AF1_x(param_3) - pp.y);
    }
    if (biU)
    {
        float param_4 = 1.0;
        w = (AF1_x(param_4) - pp.x) * pp.y;
    }
    if (biV)
    {
        w = pp.x * pp.y;
    }
    float dc = lD - lC;
    float cb = lC - lB;
    float lenX = fast::max(abs(dc), abs(cb));
    float param_5 = lenX;
    lenX = APrxLoRcpF1(param_5);
    float dirX = lD - lB;
    dir.x += (dirX * w);
    float param_6 = abs(dirX) * lenX;
    lenX = ASatF1(param_6);
    lenX *= lenX;
    len += (lenX * w);
    float ec = lE - lC;
    float ca = lC - lA;
    float lenY = fast::max(abs(ec), abs(ca));
    float param_7 = lenY;
    lenY = APrxLoRcpF1(param_7);
    float dirY = lE - lA;
    dir.y += (dirY * w);
    float param_8 = abs(dirY) * lenY;
    lenY = ASatF1(param_8);
    lenY *= lenY;
    len += (lenY * w);
}

static inline __attribute__((always_inline))
float APrxLoRsqF1(thread const float& a)
{
    uint param = 1597275508u;
    uint param_1 = 1u;
    return as_type<float>(AU1_x(param) - (as_type<uint>(a) >> AU1_x(param_1)));
}

static inline __attribute__((always_inline))
float3 AMin3F3(thread const float3& x, thread const float3& y, thread const float3& z)
{
    return fast::min(x, fast::min(y, z));
}

static inline __attribute__((always_inline))
float3 AMax3F3(thread const float3& x, thread const float3& y, thread const float3& z)
{
    return fast::max(x, fast::max(y, z));
}

static inline __attribute__((always_inline))
float3 AF3_x(thread const float& a)
{
    return float3(a, a, a);
}

static inline __attribute__((always_inline))
void FsrEasuTapF(thread float3& aC, thread float& aW, thread const float2& off, thread const float2& dir, thread const float2& len, thread const float& lob, thread const float& clp, thread const float3& c)
{
    float2 v;
    v.x = (off.x * dir.x) + (off.y * dir.y);
    v.y = (off.x * (-dir.y)) + (off.y * dir.x);
    v *= len;
    float d2 = (v.x * v.x) + (v.y * v.y);
    d2 = fast::min(d2, clp);
    float param = 0.4000000059604644775390625;
    float param_1 = -1.0;
    float wB = (AF1_x(param) * d2) + AF1_x(param_1);
    float param_2 = -1.0;
    float wA = (lob * d2) + AF1_x(param_2);
    wB *= wB;
    wA *= wA;
    float param_3 = 1.5625;
    float param_4 = -0.5625;
    wB = (AF1_x(param_3) * wB) + AF1_x(param_4);
    float w = wB * wA;
    aC += (c * w);
    aW += w;
}

static inline __attribute__((always_inline))
float ARcpF1(thread const float& x)
{
    float param = 1.0;
    return AF1_x(param) / x;
}

static inline __attribute__((always_inline))
void FsrEasuF(thread float3& pix, thread const uint2& ip, thread const uint4& con0, thread const uint4& con1, thread const uint4& con2, thread const uint4& con3, texture2d<float> InputTexture, sampler InputSampler)
{
    float2 pp = (float2(ip) * as_type<float2>(uint2(con0.xy))) + as_type<float2>(uint2(con0.zw));
    float2 fp = floor(pp);
    pp -= fp;
    float2 p0 = (fp * as_type<float2>(uint2(con1.xy))) + as_type<float2>(uint2(con1.zw));
    float2 p1 = p0 + as_type<float2>(uint2(con2.xy));
    float2 p2 = p0 + as_type<float2>(uint2(con2.zw));
    float2 p3 = p0 + as_type<float2>(uint2(con3.xy));
    float2 param = p0;
    float4 bczzR = FsrEasuRF(param, InputTexture, InputSampler);
    float2 param_1 = p0;
    float4 bczzG = FsrEasuGF(param_1, InputTexture, InputSampler);
    float2 param_2 = p0;
    float4 bczzB = FsrEasuBF(param_2, InputTexture, InputSampler);
    float2 param_3 = p1;
    float4 ijfeR = FsrEasuRF(param_3, InputTexture, InputSampler);
    float2 param_4 = p1;
    float4 ijfeG = FsrEasuGF(param_4, InputTexture, InputSampler);
    float2 param_5 = p1;
    float4 ijfeB = FsrEasuBF(param_5, InputTexture, InputSampler);
    float2 param_6 = p2;
    float4 klhgR = FsrEasuRF(param_6, InputTexture, InputSampler);
    float2 param_7 = p2;
    float4 klhgG = FsrEasuGF(param_7, InputTexture, InputSampler);
    float2 param_8 = p2;
    float4 klhgB = FsrEasuBF(param_8, InputTexture, InputSampler);
    float2 param_9 = p3;
    float4 zzonR = FsrEasuRF(param_9, InputTexture, InputSampler);
    float2 param_10 = p3;
    float4 zzonG = FsrEasuGF(param_10, InputTexture, InputSampler);
    float2 param_11 = p3;
    float4 zzonB = FsrEasuBF(param_11, InputTexture, InputSampler);
    float param_12 = 0.5;
    float param_13 = 0.5;
    float4 bczzL = (bczzB * AF4_x(param_12)) + ((bczzR * AF4_x(param_13)) + bczzG);
    float param_14 = 0.5;
    float param_15 = 0.5;
    float4 ijfeL = (ijfeB * AF4_x(param_14)) + ((ijfeR * AF4_x(param_15)) + ijfeG);
    float param_16 = 0.5;
    float param_17 = 0.5;
    float4 klhgL = (klhgB * AF4_x(param_16)) + ((klhgR * AF4_x(param_17)) + klhgG);
    float param_18 = 0.5;
    float param_19 = 0.5;
    float4 zzonL = (zzonB * AF4_x(param_18)) + ((zzonR * AF4_x(param_19)) + zzonG);
    float bL = bczzL.x;
    float cL = bczzL.y;
    float iL = ijfeL.x;
    float jL = ijfeL.y;
    float fL = ijfeL.z;
    float eL = ijfeL.w;
    float kL = klhgL.x;
    float lL = klhgL.y;
    float hL = klhgL.z;
    float gL = klhgL.w;
    float oL = zzonL.z;
    float nL = zzonL.w;
    float param_20 = 0.0;
    float2 dir = AF2_x(param_20);
    float param_21 = 0.0;
    float len = AF1_x(param_21);
    float2 param_22 = dir;
    float param_23 = len;
    float2 param_24 = pp;
    bool param_25 = true;
    bool param_26 = false;
    bool param_27 = false;
    bool param_28 = false;
    float param_29 = bL;
    float param_30 = eL;
    float param_31 = fL;
    float param_32 = gL;
    float param_33 = jL;
    FsrEasuSetF(param_22, param_23, param_24, param_25, param_26, param_27, param_28, param_29, param_30, param_31, param_32, param_33);
    dir = param_22;
    len = param_23;
    float2 param_34 = dir;
    float param_35 = len;
    float2 param_36 = pp;
    bool param_37 = false;
    bool param_38 = true;
    bool param_39 = false;
    bool param_40 = false;
    float param_41 = cL;
    float param_42 = fL;
    float param_43 = gL;
    float param_44 = hL;
    float param_45 = kL;
    FsrEasuSetF(param_34, param_35, param_36, param_37, param_38, param_39, param_40, param_41, param_42, param_43, param_44, param_45);
    dir = param_34;
    len = param_35;
    float2 param_46 = dir;
    float param_47 = len;
    float2 param_48 = pp;
    bool param_49 = false;
    bool param_50 = false;
    bool param_51 = true;
    bool param_52 = false;
    float param_53 = fL;
    float param_54 = iL;
    float param_55 = jL;
    float param_56 = kL;
    float param_57 = nL;
    FsrEasuSetF(param_46, param_47, param_48, param_49, param_50, param_51, param_52, param_53, param_54, param_55, param_56, param_57);
    dir = param_46;
    len = param_47;
    float2 param_58 = dir;
    float param_59 = len;
    float2 param_60 = pp;
    bool param_61 = false;
    bool param_62 = false;
    bool param_63 = false;
    bool param_64 = true;
    float param_65 = gL;
    float param_66 = jL;
    float param_67 = kL;
    float param_68 = lL;
    float param_69 = oL;
    FsrEasuSetF(param_58, param_59, param_60, param_61, param_62, param_63, param_64, param_65, param_66, param_67, param_68, param_69);
    dir = param_58;
    len = param_59;
    float2 dir2 = dir * dir;
    float dirR = dir2.x + dir2.y;
    float param_70 = 3.0517578125e-05;
    bool zro = dirR < AF1_x(param_70);
    float param_71 = dirR;
    dirR = APrxLoRsqF1(param_71);
    float _817;
    if (zro)
    {
        float param_72 = 1.0;
        _817 = AF1_x(param_72);
    }
    else
    {
        _817 = dirR;
    }
    dirR = _817;
    float _826;
    if (zro)
    {
        float param_73 = 1.0;
        _826 = AF1_x(param_73);
    }
    else
    {
        _826 = dir.x;
    }
    dir.x = _826;
    float param_74 = dirR;
    dir *= AF2_x(param_74);
    float param_75 = 0.5;
    len *= AF1_x(param_75);
    len *= len;
    float param_76 = fast::max(abs(dir.x), abs(dir.y));
    float stretch = ((dir.x * dir.x) + (dir.y * dir.y)) * APrxLoRcpF1(param_76);
    float param_77 = 1.0;
    float param_78 = 1.0;
    float param_79 = 1.0;
    float param_80 = -0.5;
    float2 len2 = float2(AF1_x(param_77) + ((stretch - AF1_x(param_78)) * len), AF1_x(param_79) + (AF1_x(param_80) * len));
    float param_81 = 0.5;
    float param_82 = -0.2899999916553497314453125;
    float lob = AF1_x(param_81) + (AF1_x(param_82) * len);
    float param_83 = lob;
    float clp = APrxLoRcpF1(param_83);
    float3 param_84 = float3(ijfeR.z, ijfeG.z, ijfeB.z);
    float3 param_85 = float3(klhgR.w, klhgG.w, klhgB.w);
    float3 param_86 = float3(ijfeR.y, ijfeG.y, ijfeB.y);
    float3 min4 = fast::min(AMin3F3(param_84, param_85, param_86), float3(klhgR.x, klhgG.x, klhgB.x));
    float3 param_87 = float3(ijfeR.z, ijfeG.z, ijfeB.z);
    float3 param_88 = float3(klhgR.w, klhgG.w, klhgB.w);
    float3 param_89 = float3(ijfeR.y, ijfeG.y, ijfeB.y);
    float3 max4 = fast::max(AMax3F3(param_87, param_88, param_89), float3(klhgR.x, klhgG.x, klhgB.x));
    float param_90 = 0.0;
    float3 aC = AF3_x(param_90);
    float param_91 = 0.0;
    float aW = AF1_x(param_91);
    float3 param_92 = aC;
    float param_93 = aW;
    float2 param_94 = float2(0.0, -1.0) - pp;
    float2 param_95 = dir;
    float2 param_96 = len2;
    float param_97 = lob;
    float param_98 = clp;
    float3 param_99 = float3(bczzR.x, bczzG.x, bczzB.x);
    FsrEasuTapF(param_92, param_93, param_94, param_95, param_96, param_97, param_98, param_99);
    aC = param_92;
    aW = param_93;
    float3 param_100 = aC;
    float param_101 = aW;
    float2 param_102 = float2(1.0, -1.0) - pp;
    float2 param_103 = dir;
    float2 param_104 = len2;
    float param_105 = lob;
    float param_106 = clp;
    float3 param_107 = float3(bczzR.y, bczzG.y, bczzB.y);
    FsrEasuTapF(param_100, param_101, param_102, param_103, param_104, param_105, param_106, param_107);
    aC = param_100;
    aW = param_101;
    float3 param_108 = aC;
    float param_109 = aW;
    float2 param_110 = float2(-1.0, 1.0) - pp;
    float2 param_111 = dir;
    float2 param_112 = len2;
    float param_113 = lob;
    float param_114 = clp;
    float3 param_115 = float3(ijfeR.x, ijfeG.x, ijfeB.x);
    FsrEasuTapF(param_108, param_109, param_110, param_111, param_112, param_113, param_114, param_115);
    aC = param_108;
    aW = param_109;
    float3 param_116 = aC;
    float param_117 = aW;
    float2 param_118 = float2(0.0, 1.0) - pp;
    float2 param_119 = dir;
    float2 param_120 = len2;
    float param_121 = lob;
    float param_122 = clp;
    float3 param_123 = float3(ijfeR.y, ijfeG.y, ijfeB.y);
    FsrEasuTapF(param_116, param_117, param_118, param_119, param_120, param_121, param_122, param_123);
    aC = param_116;
    aW = param_117;
    float3 param_124 = aC;
    float param_125 = aW;
    float2 param_126 = float2(0.0) - pp;
    float2 param_127 = dir;
    float2 param_128 = len2;
    float param_129 = lob;
    float param_130 = clp;
    float3 param_131 = float3(ijfeR.z, ijfeG.z, ijfeB.z);
    FsrEasuTapF(param_124, param_125, param_126, param_127, param_128, param_129, param_130, param_131);
    aC = param_124;
    aW = param_125;
    float3 param_132 = aC;
    float param_133 = aW;
    float2 param_134 = float2(-1.0, 0.0) - pp;
    float2 param_135 = dir;
    float2 param_136 = len2;
    float param_137 = lob;
    float param_138 = clp;
    float3 param_139 = float3(ijfeR.w, ijfeG.w, ijfeB.w);
    FsrEasuTapF(param_132, param_133, param_134, param_135, param_136, param_137, param_138, param_139);
    aC = param_132;
    aW = param_133;
    float3 param_140 = aC;
    float param_141 = aW;
    float2 param_142 = float2(1.0) - pp;
    float2 param_143 = dir;
    float2 param_144 = len2;
    float param_145 = lob;
    float param_146 = clp;
    float3 param_147 = float3(klhgR.x, klhgG.x, klhgB.x);
    FsrEasuTapF(param_140, param_141, param_142, param_143, param_144, param_145, param_146, param_147);
    aC = param_140;
    aW = param_141;
    float3 param_148 = aC;
    float param_149 = aW;
    float2 param_150 = float2(2.0, 1.0) - pp;
    float2 param_151 = dir;
    float2 param_152 = len2;
    float param_153 = lob;
    float param_154 = clp;
    float3 param_155 = float3(klhgR.y, klhgG.y, klhgB.y);
    FsrEasuTapF(param_148, param_149, param_150, param_151, param_152, param_153, param_154, param_155);
    aC = param_148;
    aW = param_149;
    float3 param_156 = aC;
    float param_157 = aW;
    float2 param_158 = float2(2.0, 0.0) - pp;
    float2 param_159 = dir;
    float2 param_160 = len2;
    float param_161 = lob;
    float param_162 = clp;
    float3 param_163 = float3(klhgR.z, klhgG.z, klhgB.z);
    FsrEasuTapF(param_156, param_157, param_158, param_159, param_160, param_161, param_162, param_163);
    aC = param_156;
    aW = param_157;
    float3 param_164 = aC;
    float param_165 = aW;
    float2 param_166 = float2(1.0, 0.0) - pp;
    float2 param_167 = dir;
    float2 param_168 = len2;
    float param_169 = lob;
    float param_170 = clp;
    float3 param_171 = float3(klhgR.w, klhgG.w, klhgB.w);
    FsrEasuTapF(param_164, param_165, param_166, param_167, param_168, param_169, param_170, param_171);
    aC = param_164;
    aW = param_165;
    float3 param_172 = aC;
    float param_173 = aW;
    float2 param_174 = float2(1.0, 2.0) - pp;
    float2 param_175 = dir;
    float2 param_176 = len2;
    float param_177 = lob;
    float param_178 = clp;
    float3 param_179 = float3(zzonR.z, zzonG.z, zzonB.z);
    FsrEasuTapF(param_172, param_173, param_174, param_175, param_176, param_177, param_178, param_179);
    aC = param_172;
    aW = param_173;
    float3 param_180 = aC;
    float param_181 = aW;
    float2 param_182 = float2(0.0, 2.0) - pp;
    float2 param_183 = dir;
    float2 param_184 = len2;
    float param_185 = lob;
    float param_186 = clp;
    float3 param_187 = float3(zzonR.w, zzonG.w, zzonB.w);
    FsrEasuTapF(param_180, param_181, param_182, param_183, param_184, param_185, param_186, param_187);
    aC = param_180;
    aW = param_181;
    float param_188 = aW;
    float param_189 = ARcpF1(param_188);
    pix = fast::min(max4, fast::max(min4, aC * AF3_x(param_189)));
}

static inline __attribute__((always_inline))
void Filter(thread const uint2& pos, texture2d<float> InputTexture, sampler InputSampler, constant constantsBuffer& v_1315, texture2d<float, access::write> OutputTexture)
{
    uint2 param_1 = pos;
    uint4 param_2 = v_1315.const0;
    uint4 param_3 = v_1315.const1;
    uint4 param_4 = v_1315.const2;
    uint4 param_5 = v_1315.const3;
    float3 param;
    FsrEasuF(param, param_1, param_2, param_3, param_4, param_5, InputTexture, InputSampler);
    float3 c = param;
    if (v_1315.Sample.x == 1u)
    {
        c *= c;
    }
    OutputTexture.write(float4(c, 1.0), uint2(int2(pos)));
}

kernel void main0(constant spvDescriptorSetBuffer0& spvDescriptorSet0 [[buffer(0)]], uint3 gl_LocalInvocationID [[thread_position_in_threadgroup]], uint3 gl_WorkGroupID [[threadgroup_position_in_grid]])
{
    uint param = gl_LocalInvocationID.x;
    uint2 gxy = ARmp8x8(param) + uint2(gl_WorkGroupID.x << 4u, gl_WorkGroupID.y << 4u);
    uint2 param_1 = gxy;
    Filter(param_1, spvDescriptorSet0.InputTexture, textureSampler, (*spvDescriptorSet0.v_1315), spvDescriptorSet0.OutputTexture);
    gxy.x += 8u;
    uint2 param_2 = gxy;
    Filter(param_2, spvDescriptorSet0.InputTexture, textureSampler, (*spvDescriptorSet0.v_1315), spvDescriptorSet0.OutputTexture);
    gxy.y += 8u;
    uint2 param_3 = gxy;
    Filter(param_3, spvDescriptorSet0.InputTexture, textureSampler, (*spvDescriptorSet0.v_1315), spvDescriptorSet0.OutputTexture);
    gxy.x -= 8u;
    uint2 param_4 = gxy;
    Filter(param_4, spvDescriptorSet0.InputTexture, textureSampler, (*spvDescriptorSet0.v_1315), spvDescriptorSet0.OutputTexture);
}
