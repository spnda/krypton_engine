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
    texture2d<float, access::sample> inputTexture [[id(0)]];
    constant constantsBuffer* v_1281 [[id(1)]];
    texture2d<float, access::write> outputTexture [[id(2)]];
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
half4 FsrEasuRH(thread const float2& p, texture2d<float> inputTexture, sampler inputSampler)
{
    half4 res = half4(inputTexture.gather(inputSampler, p, int2(0), component::x));
    return res;
}

static inline __attribute__((always_inline))
half4 FsrEasuGH(thread const float2& p, texture2d<float> inputTexture, sampler inputSampler)
{
    half4 res = half4(inputTexture.gather(inputSampler, p, int2(0), component::y));
    return res;
}

static inline __attribute__((always_inline))
half4 FsrEasuBH(thread const float2& p, texture2d<float> inputTexture, sampler inputSampler)
{
    half4 res = half4(inputTexture.gather(inputSampler, p, int2(0), component::z));
    return res;
}

static inline __attribute__((always_inline))
half4 AH4_x(thread const half& a)
{
    return half4(a, a, a, a);
}

static inline __attribute__((always_inline))
half2 AH2_x(thread const half& a)
{
    return half2(a, a);
}

static inline __attribute__((always_inline))
half AH1_x(thread const half& a)
{
    return a;
}

static inline __attribute__((always_inline))
half2 ARcpH2(thread const half2& x)
{
    half param = half(1.0);
    return AH2_x(param) / x;
}

static inline __attribute__((always_inline))
half2 ASatH2(thread const half2& x)
{
    half param = half(0.0);
    half param_1 = half(1.0);
    return clamp(x, AH2_x(param), AH2_x(param_1));
}

static inline __attribute__((always_inline))
void FsrEasuSetH(thread half2& dirPX, thread half2& dirPY, thread half2& lenP, thread const half2& pp, thread const bool& biST, thread const bool& biUV, thread const half2& lA, thread const half2& lB, thread const half2& lC, thread const half2& lD, thread const half2& lE)
{
    half param = half(0.0);
    half2 w = AH2_x(param);
    if (biST)
    {
        half param_1 = half(1.0);
        half param_2 = AH1_x(param_1) - pp.y;
        w = (half2(half(1.0), half(0.0)) + half2(-pp.x, pp.x)) * AH2_x(param_2);
    }
    if (biUV)
    {
        half param_3 = pp.y;
        w = (half2(half(1.0), half(0.0)) + half2(-pp.x, pp.x)) * AH2_x(param_3);
    }
    half2 dc = lD - lC;
    half2 cb = lC - lB;
    half2 lenX = max(abs(dc), abs(cb));
    half2 param_4 = lenX;
    lenX = ARcpH2(param_4);
    half2 dirX = lD - lB;
    dirPX += (dirX * w);
    half2 param_5 = abs(dirX) * lenX;
    lenX = ASatH2(param_5);
    lenX *= lenX;
    lenP += (lenX * w);
    half2 ec = lE - lC;
    half2 ca = lC - lA;
    half2 lenY = max(abs(ec), abs(ca));
    half2 param_6 = lenY;
    lenY = ARcpH2(param_6);
    half2 dirY = lE - lA;
    dirPY += (dirY * w);
    half2 param_7 = abs(dirY) * lenY;
    lenY = ASatH2(param_7);
    lenY *= lenY;
    lenP += (lenY * w);
}

static inline __attribute__((always_inline))
ushort AW1_x(thread const ushort& a)
{
    return a;
}

static inline __attribute__((always_inline))
half APrxLoRsqH1(thread const half& a)
{
    ushort param = ushort(22947);
    ushort param_1 = ushort(1);
    return half(AW1_x(param) - (as_type<ushort>(a) >> AW1_x(param_1)));
    // return as_type<half>(AW1_x(param) - (as_type<ushort>(a) >> AW1_x(param_1)));
}

static inline __attribute__((always_inline))
half APrxLoRcpH1(thread const half& a)
{
    ushort param = ushort(30596);
    return half(AW1_x(param) - as_type<ushort>(a));
    // return as_type<half>(AW1_x(param) - as_type<ushort>(a));
}

static inline __attribute__((always_inline))
void FsrEasuTapH(thread half2& aCR, thread half2& aCG, thread half2& aCB, thread half2& aW, thread const half2& offX, thread const half2& offY, thread const half2& dir, thread const half2& len, thread const half& lob, thread const half& clp, thread const half2& cR, thread const half2& cG, thread const half2& cB)
{
    half2 vX = (offX * dir.xx) + (offY * dir.yy);
    half2 vY = (offX * (-dir.yy)) + (offY * dir.xx);
    vX *= len.x;
    vY *= len.y;
    half2 d2 = (vX * vX) + (vY * vY);
    half param = clp;
    d2 = min(d2, AH2_x(param));
    half param_1 = half(0.39990234375);
    half param_2 = half(-1.0);
    half2 wB = (AH2_x(param_1) * d2) + AH2_x(param_2);
    half param_3 = lob;
    half param_4 = half(-1.0);
    half2 wA = (AH2_x(param_3) * d2) + AH2_x(param_4);
    wB *= wB;
    wA *= wA;
    half param_5 = half(1.5625);
    half param_6 = half(-0.5625);
    wB = (AH2_x(param_5) * wB) + AH2_x(param_6);
    half2 w = wB * wA;
    aCR += (cR * w);
    aCG += (cG * w);
    aCB += (cB * w);
    aW += w;
}

static inline __attribute__((always_inline))
half ARcpH1(thread const half& x)
{
    half param = half(1.0);
    return AH1_x(param) / x;
}

static inline __attribute__((always_inline))
half3 AH3_x(thread const half& a)
{
    return half3(a, a, a);
}

static inline __attribute__((always_inline))
void FsrEasuH(thread half3& pix, thread const uint2& ip, thread const uint4& con0, thread const uint4& con1, thread const uint4& con2, thread const uint4& con3, texture2d<float> inputTexture, sampler inputSampler)
{
    float2 pp = (float2(ip) * as_type<float2>(uint2(con0.xy))) + as_type<float2>(uint2(con0.zw));
    float2 fp = floor(pp);
    pp -= fp;
    half2 ppp = half2(pp);
    float2 p0 = (fp * as_type<float2>(uint2(con1.xy))) + as_type<float2>(uint2(con1.zw));
    float2 p1 = p0 + as_type<float2>(uint2(con2.xy));
    float2 p2 = p0 + as_type<float2>(uint2(con2.zw));
    float2 p3 = p0 + as_type<float2>(uint2(con3.xy));
    float2 param = p0;
    half4 bczzR = FsrEasuRH(param, inputTexture, inputSampler);
    float2 param_1 = p0;
    half4 bczzG = FsrEasuGH(param_1, inputTexture, inputSampler);
    float2 param_2 = p0;
    half4 bczzB = FsrEasuBH(param_2, inputTexture, inputSampler);
    float2 param_3 = p1;
    half4 ijfeR = FsrEasuRH(param_3, inputTexture, inputSampler);
    float2 param_4 = p1;
    half4 ijfeG = FsrEasuGH(param_4, inputTexture, inputSampler);
    float2 param_5 = p1;
    half4 ijfeB = FsrEasuBH(param_5, inputTexture, inputSampler);
    float2 param_6 = p2;
    half4 klhgR = FsrEasuRH(param_6, inputTexture, inputSampler);
    float2 param_7 = p2;
    half4 klhgG = FsrEasuGH(param_7, inputTexture, inputSampler);
    float2 param_8 = p2;
    half4 klhgB = FsrEasuBH(param_8, inputTexture, inputSampler);
    float2 param_9 = p3;
    half4 zzonR = FsrEasuRH(param_9, inputTexture, inputSampler);
    float2 param_10 = p3;
    half4 zzonG = FsrEasuGH(param_10, inputTexture, inputSampler);
    float2 param_11 = p3;
    half4 zzonB = FsrEasuBH(param_11, inputTexture, inputSampler);
    half param_12 = half(0.5);
    half param_13 = half(0.5);
    half4 bczzL = (bczzB * AH4_x(param_12)) + ((bczzR * AH4_x(param_13)) + bczzG);
    half param_14 = half(0.5);
    half param_15 = half(0.5);
    half4 ijfeL = (ijfeB * AH4_x(param_14)) + ((ijfeR * AH4_x(param_15)) + ijfeG);
    half param_16 = half(0.5);
    half param_17 = half(0.5);
    half4 klhgL = (klhgB * AH4_x(param_16)) + ((klhgR * AH4_x(param_17)) + klhgG);
    half param_18 = half(0.5);
    half param_19 = half(0.5);
    half4 zzonL = (zzonB * AH4_x(param_18)) + ((zzonR * AH4_x(param_19)) + zzonG);
    half bL = bczzL.x;
    half cL = bczzL.y;
    half iL = ijfeL.x;
    half jL = ijfeL.y;
    half fL = ijfeL.z;
    half eL = ijfeL.w;
    half kL = klhgL.x;
    half lL = klhgL.y;
    half hL = klhgL.z;
    half gL = klhgL.w;
    half oL = zzonL.z;
    half nL = zzonL.w;
    half param_20 = half(0.0);
    half2 dirPX = AH2_x(param_20);
    half param_21 = half(0.0);
    half2 dirPY = AH2_x(param_21);
    half param_22 = half(0.0);
    half2 lenP = AH2_x(param_22);
    half2 param_23 = dirPX;
    half2 param_24 = dirPY;
    half2 param_25 = lenP;
    half2 param_26 = ppp;
    bool param_27 = true;
    bool param_28 = false;
    half2 param_29 = half2(bL, cL);
    half2 param_30 = half2(eL, fL);
    half2 param_31 = half2(fL, gL);
    half2 param_32 = half2(gL, hL);
    half2 param_33 = half2(jL, kL);
    FsrEasuSetH(param_23, param_24, param_25, param_26, param_27, param_28, param_29, param_30, param_31, param_32, param_33);
    dirPX = param_23;
    dirPY = param_24;
    lenP = param_25;
    half2 param_34 = dirPX;
    half2 param_35 = dirPY;
    half2 param_36 = lenP;
    half2 param_37 = ppp;
    bool param_38 = false;
    bool param_39 = true;
    half2 param_40 = half2(fL, gL);
    half2 param_41 = half2(iL, jL);
    half2 param_42 = half2(jL, kL);
    half2 param_43 = half2(kL, lL);
    half2 param_44 = half2(nL, oL);
    FsrEasuSetH(param_34, param_35, param_36, param_37, param_38, param_39, param_40, param_41, param_42, param_43, param_44);
    dirPX = param_34;
    dirPY = param_35;
    lenP = param_36;
    half2 dir = half2(dirPX.x + dirPX.y, dirPY.x + dirPY.y);
    half len = lenP.x + lenP.y;
    half2 dir2 = dir * dir;
    half dirR = dir2.x + dir2.y;
    half param_45 = half(3.0517578125e-05);
    bool zro = dirR < AH1_x(param_45);
    half param_46 = dirR;
    dirR = APrxLoRsqH1(param_46);
    half _812;
    if (zro)
    {
        half param_47 = half(1.0);
        _812 = AH1_x(param_47);
    }
    else
    {
        _812 = dirR;
    }
    dirR = _812;
    half _821;
    if (zro)
    {
        half param_48 = half(1.0);
        _821 = AH1_x(param_48);
    }
    else
    {
        _821 = dir.x;
    }
    dir.x = _821;
    half param_49 = dirR;
    dir *= AH2_x(param_49);
    half param_50 = half(0.5);
    len *= AH1_x(param_50);
    len *= len;
    half param_51 = max(abs(dir.x), abs(dir.y));
    half stretch = ((dir.x * dir.x) + (dir.y * dir.y)) * APrxLoRcpH1(param_51);
    half param_52 = half(1.0);
    half param_53 = half(1.0);
    half param_54 = half(1.0);
    half param_55 = half(-0.5);
    half2 len2 = half2(AH1_x(param_52) + ((stretch - AH1_x(param_53)) * len), AH1_x(param_54) + (AH1_x(param_55) * len));
    half param_56 = half(0.5);
    half param_57 = half(-0.289794921875);
    half lob = AH1_x(param_56) + (AH1_x(param_57) * len);
    half param_58 = lob;
    half clp = APrxLoRcpH1(param_58);
    half2 bothR = max(max(half2(-ijfeR.z, ijfeR.z), half2(-klhgR.w, klhgR.w)), max(half2(-ijfeR.y, ijfeR.y), half2(-klhgR.x, klhgR.x)));
    half2 bothG = max(max(half2(-ijfeG.z, ijfeG.z), half2(-klhgG.w, klhgG.w)), max(half2(-ijfeG.y, ijfeG.y), half2(-klhgG.x, klhgG.x)));
    half2 bothB = max(max(half2(-ijfeB.z, ijfeB.z), half2(-klhgB.w, klhgB.w)), max(half2(-ijfeB.y, ijfeB.y), half2(-klhgB.x, klhgB.x)));
    half param_59 = half(0.0);
    half2 pR = AH2_x(param_59);
    half param_60 = half(0.0);
    half2 pG = AH2_x(param_60);
    half param_61 = half(0.0);
    half2 pB = AH2_x(param_61);
    half param_62 = half(0.0);
    half2 pW = AH2_x(param_62);
    half2 param_63 = pR;
    half2 param_64 = pG;
    half2 param_65 = pB;
    half2 param_66 = pW;
    half2 param_67 = half2(half(0.0), half(1.0)) - ppp.xx;
    half2 param_68 = half2(half(-1.0)) - ppp.yy;
    half2 param_69 = dir;
    half2 param_70 = len2;
    half param_71 = lob;
    half param_72 = clp;
    half2 param_73 = bczzR.xy;
    half2 param_74 = bczzG.xy;
    half2 param_75 = bczzB.xy;
    FsrEasuTapH(param_63, param_64, param_65, param_66, param_67, param_68, param_69, param_70, param_71, param_72, param_73, param_74, param_75);
    pR = param_63;
    pG = param_64;
    pB = param_65;
    pW = param_66;
    half2 param_76 = pR;
    half2 param_77 = pG;
    half2 param_78 = pB;
    half2 param_79 = pW;
    half2 param_80 = half2(half(-1.0), half(0.0)) - ppp.xx;
    half2 param_81 = half2(half(1.0)) - ppp.yy;
    half2 param_82 = dir;
    half2 param_83 = len2;
    half param_84 = lob;
    half param_85 = clp;
    half2 param_86 = ijfeR.xy;
    half2 param_87 = ijfeG.xy;
    half2 param_88 = ijfeB.xy;
    FsrEasuTapH(param_76, param_77, param_78, param_79, param_80, param_81, param_82, param_83, param_84, param_85, param_86, param_87, param_88);
    pR = param_76;
    pG = param_77;
    pB = param_78;
    pW = param_79;
    half2 param_89 = pR;
    half2 param_90 = pG;
    half2 param_91 = pB;
    half2 param_92 = pW;
    half2 param_93 = half2(half(0.0), half(-1.0)) - ppp.xx;
    half2 param_94 = half2(half(0.0)) - ppp.yy;
    half2 param_95 = dir;
    half2 param_96 = len2;
    half param_97 = lob;
    half param_98 = clp;
    half2 param_99 = ijfeR.zw;
    half2 param_100 = ijfeG.zw;
    half2 param_101 = ijfeB.zw;
    FsrEasuTapH(param_89, param_90, param_91, param_92, param_93, param_94, param_95, param_96, param_97, param_98, param_99, param_100, param_101);
    pR = param_89;
    pG = param_90;
    pB = param_91;
    pW = param_92;
    half2 param_102 = pR;
    half2 param_103 = pG;
    half2 param_104 = pB;
    half2 param_105 = pW;
    half2 param_106 = half2(half(1.0), half(2.0)) - ppp.xx;
    half2 param_107 = half2(half(1.0)) - ppp.yy;
    half2 param_108 = dir;
    half2 param_109 = len2;
    half param_110 = lob;
    half param_111 = clp;
    half2 param_112 = klhgR.xy;
    half2 param_113 = klhgG.xy;
    half2 param_114 = klhgB.xy;
    FsrEasuTapH(param_102, param_103, param_104, param_105, param_106, param_107, param_108, param_109, param_110, param_111, param_112, param_113, param_114);
    pR = param_102;
    pG = param_103;
    pB = param_104;
    pW = param_105;
    half2 param_115 = pR;
    half2 param_116 = pG;
    half2 param_117 = pB;
    half2 param_118 = pW;
    half2 param_119 = half2(half(2.0), half(1.0)) - ppp.xx;
    half2 param_120 = half2(half(0.0)) - ppp.yy;
    half2 param_121 = dir;
    half2 param_122 = len2;
    half param_123 = lob;
    half param_124 = clp;
    half2 param_125 = klhgR.zw;
    half2 param_126 = klhgG.zw;
    half2 param_127 = klhgB.zw;
    FsrEasuTapH(param_115, param_116, param_117, param_118, param_119, param_120, param_121, param_122, param_123, param_124, param_125, param_126, param_127);
    pR = param_115;
    pG = param_116;
    pB = param_117;
    pW = param_118;
    half2 param_128 = pR;
    half2 param_129 = pG;
    half2 param_130 = pB;
    half2 param_131 = pW;
    half2 param_132 = half2(half(1.0), half(0.0)) - ppp.xx;
    half2 param_133 = half2(half(2.0)) - ppp.yy;
    half2 param_134 = dir;
    half2 param_135 = len2;
    half param_136 = lob;
    half param_137 = clp;
    half2 param_138 = zzonR.zw;
    half2 param_139 = zzonG.zw;
    half2 param_140 = zzonB.zw;
    FsrEasuTapH(param_128, param_129, param_130, param_131, param_132, param_133, param_134, param_135, param_136, param_137, param_138, param_139, param_140);
    pR = param_128;
    pG = param_129;
    pB = param_130;
    pW = param_131;
    half3 aC = half3(pR.x + pR.y, pG.x + pG.y, pB.x + pB.y);
    half aW = pW.x + pW.y;
    half param_141 = aW;
    half param_142 = ARcpH1(param_141);
    pix = min(half3(bothR.y, bothG.y, bothB.y), max(-half3(bothR.x, bothG.x, bothB.x), aC * AH3_x(param_142)));
}

static inline __attribute__((always_inline))
void Filter(thread const uint2& pos, texture2d<float> inputTexture, sampler inputSampler, constant constantsBuffer& v_1281, texture2d<float, access::write> outputTexture)
{
    uint2 param_1 = pos;
    uint4 param_2 = v_1281.const0;
    uint4 param_3 = v_1281.const1;
    uint4 param_4 = v_1281.const2;
    uint4 param_5 = v_1281.const3;
    half3 param;
    FsrEasuH(param, param_1, param_2, param_3, param_4, param_5, inputTexture, inputSampler);
    half3 c = param;
    if (v_1281.Sample.x == 1u)
    {
        c *= c;
    }
    outputTexture.write(float4(half4(c, half(1.0))), uint2(int2(pos)));
}

kernel void main0(constant spvDescriptorSetBuffer0& spvDescriptorSet0 [[buffer(0)]], uint3 gl_LocalInvocationID [[thread_position_in_threadgroup]], uint3 gl_WorkGroupID [[threadgroup_position_in_grid]])
{
    uint param = gl_LocalInvocationID.x;
    uint2 gxy = ARmp8x8(param) + uint2(gl_WorkGroupID.x << 4u, gl_WorkGroupID.y << 4u);
    uint2 param_1 = gxy;
    Filter(param_1, spvDescriptorSet0.inputTexture, textureSampler, (*spvDescriptorSet0.v_1281), spvDescriptorSet0.outputTexture);
    gxy.x += 8u;
    uint2 param_2 = gxy;
    Filter(param_2, spvDescriptorSet0.inputTexture, textureSampler, (*spvDescriptorSet0.v_1281), spvDescriptorSet0.outputTexture);
    gxy.y += 8u;
    uint2 param_3 = gxy;
    Filter(param_3, spvDescriptorSet0.inputTexture, textureSampler, (*spvDescriptorSet0.v_1281), spvDescriptorSet0.outputTexture);
    gxy.x -= 8u;
    uint2 param_4 = gxy;
    Filter(param_4, spvDescriptorSet0.inputTexture, textureSampler, (*spvDescriptorSet0.v_1281), spvDescriptorSet0.outputTexture);
}
