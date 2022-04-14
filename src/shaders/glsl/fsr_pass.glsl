#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform constantsBuffer {
    uvec4 const0;
    uvec4 const1;
    uvec4 const2;
    uvec4 const3;
    uvec4 Sample;
};

#define A_GPU 1
#define A_GLSL 1

#define SAMPLE_SLOW_FALLBACK 1
#define SAMPLE_EASU 1

#if SAMPLE_SLOW_FALLBACK
	#include "ffx_a.h"
	layout(set=0,binding=1) uniform texture2D InputTexture;
	layout(set=0,binding=2,rgba32f) uniform image2D OutputTexture;
	layout(set=0,binding=3) uniform sampler InputSampler;
	#if SAMPLE_EASU
		#define FSR_EASU_F 1
		AF4 FsrEasuRF(AF2 p) { AF4 res = textureGather(sampler2D(InputTexture,InputSampler), p, 0); return res; }
		AF4 FsrEasuGF(AF2 p) { AF4 res = textureGather(sampler2D(InputTexture,InputSampler), p, 1); return res; }
		AF4 FsrEasuBF(AF2 p) { AF4 res = textureGather(sampler2D(InputTexture,InputSampler), p, 2); return res; }
	#endif
	#if SAMPLE_RCAS
		#define FSR_RCAS_F
		AF4 FsrRcasLoadF(ASU2 p) { return texelFetch(sampler2D(InputTexture,InputSampler), ASU2(p), 0); }
		void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
	#endif
#else
	#define A_HALF
	#include "ffx_a.h"
	layout(set=0,binding=1) uniform texture2D InputTexture;
	layout(set=0,binding=2,rgba16f) uniform image2D OutputTexture;
	layout(set=0,binding=3) uniform sampler InputSampler;
	#if SAMPLE_EASU
		#define FSR_EASU_H 1
		AH4 FsrEasuRH(AF2 p) { AH4 res = AH4(textureGather(sampler2D(InputTexture,InputSampler), p, 0)); return res; }
		AH4 FsrEasuGH(AF2 p) { AH4 res = AH4(textureGather(sampler2D(InputTexture,InputSampler), p, 1)); return res; }
		AH4 FsrEasuBH(AF2 p) { AH4 res = AH4(textureGather(sampler2D(InputTexture,InputSampler), p, 2)); return res; }
	#endif
	#if SAMPLE_RCAS
		#define FSR_RCAS_H
		AH4 FsrRcasLoadH(ASW2 p) { return AH4(texelFetch(sampler2D(InputTexture,InputSampler), ASU2(p), 0)); }
		void FsrRcasInputH(inout AH1 r,inout AH1 g,inout AH1 b){}
	#endif
#endif

#include "ffx_fsr1.h"

void Filter(AU2 pos)
{
#if SAMPLE_BILINEAR
	AF2 pp = (AF2(pos) * AF2_AU2(const0.xy) + AF2_AU2(const0.zw)) * AF2_AU2(const1.xy) + AF2(0.5, -0.5) * AF2_AU2(const1.zw);
	imageStore(OutputTexture, ASU2(pos), textureLod(sampler2D(InputTexture,InputSampler), pp, 0.0));
#endif
#if SAMPLE_EASU
	#if SAMPLE_SLOW_FALLBACK
		AF3 c;
		FsrEasuF(c, pos, const0, const1, const2, const3);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AF4(c, 1));
	#else
		AH3 c;
		FsrEasuH(c, pos, const0, const1, const2, const3);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AH4(c, 1));
	#endif
#endif
#if SAMPLE_RCAS
	#if SAMPLE_SLOW_FALLBACK
		AF3 c;
		FsrRcasF(c.r, c.g, c.b, pos, const0);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AF4(c, 1));
	#else
		AH3 c;
		FsrRcasH(c.r, c.g, c.b, pos, const0);
		if( Sample.x == 1 )
			c *= c;
		imageStore(OutputTexture, ASU2(pos), AH4(c, 1));
	#endif
#endif
}

layout(local_size_x = 64) in;
void main() {
    AU2 gxy = ARmp8x8(gl_LocalInvocationID.x) + AU2(gl_WorkGroupID.x << 4u, gl_WorkGroupID.y << 4u);
    Filter(gxy);
    gxy.x += 8u;
    Filter(gxy);
    gxy.y += 8u;
    Filter(gxy);
    gxy.x -= 8u;
    Filter(gxy);
}
