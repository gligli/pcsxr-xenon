/*
 * File:   XeRenderer.cpp
 * Author: cc
 *
 * Created on 11 octobre 2011, 16:51
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <console/console.h>

#include <ppc/timebase.h>
#include <time/time.h>
#include <time.h>

#include <debug.h>
#ifndef TR
#define TR {printf("[Trace] in function %s, line %d, file %s\n",__FUNCTION__,__LINE__,__FILE__);}
#endif
#include "GpuRenderer.h"

// instance
GpuRenderer gpuRenderer;

typedef unsigned int DWORD;
#include "psx.ps.h"
#include "psx.ps.f.h"
#include "psx.ps.g.h"
#include "psx.ps.c.h"
#include "psx.vs.h"

#include "fxaa.vs.h"
#include "fxaa.ps.h"

#define MAKE_COLOR4(r,g,b,a) ((a)<<24 | ((b)<<16) | ((g)<<8) | (r))

#define MAX_VERTEX_COUNT 65536
#define MAX_INDICE_COUNT 65536

static int nb_vertices = 0;
static int nb_indices = 0;

XenosDevice *g_xe;

static XenosDevice *xe;
static XenosDevice _xe;

static int16_t indices_strip[] = {0, 1, 2, 2, 1, 3};
static int16_t indices_quad[] = {0, 1, 2, 0, 2, 3};

struct fxaa_vb {
	float x, y, z, w; // pos
	float u, v; //tex coord
};

enum {
	UvBottom = 0,
	UvTop,
	UvLeft,
	UvRight
};
static float ScreenUv[4] = {0.f, 1.0f, 1.0f, 0.f};

extern "C" {
	void doScreenCapture();
}

struct XenosSurface * pPsx = NULL;

static int postprocessenabled = 0;
static int debug_draw = 0;

extern "C" void enablePostProcess(int enable) {
	postprocessenabled = enable;
}

void GpuRenderer::BeginPostProcess() {
	//    Xe_SetIndices(xe, pIb);
	//    Xe_SetStreamSource(xe, 0, pVb, 0, 4);
	//    Xe_SetRenderTarget(xe, pPostRenderSurface);
};

void GpuRenderer::EndPostProcess() {
	//Xe_SetRenderTarget(xe, pRenderSurface);
};

static GpuTex * psxFmvSurf = NULL;
static GpuTex * psxVramSurf = NULL;

GpuTex * GetPsxVramSurf() {
	if (psxVramSurf == NULL) {
		psxVramSurf = gpuRenderer.CreateTexture(2048, 2048, FMT_A8R8G8B8);
	}
	return psxVramSurf;
}

GpuTex * GetFmvSurf() {
	if (psxFmvSurf == NULL) {
		psxFmvSurf = gpuRenderer.CreateTexture(1024, 1024, FMT_A8R8G8B8);
	}
	return psxFmvSurf;
}

void gui_vsync();

/**
 * Slow
 */
void GpuRenderer::RenderPostProcess() {
	if (postprocessenabled) {
		//Xe_Resolve(xe);
		Xe_ResolveInto(xe, pPostRenderSurface, XE_SOURCE_COLOR, XE_CLEAR_DS);
		Xe_Sync(xe);

		Xe_InvalidateState(xe);

		EndPostProcess();
		Xe_SetCullMode(xe, XE_CULL_NONE);

		// set shaders
		Xe_SetShader(xe, SHADER_TYPE_PIXEL, g_pPixelShaderPost, 0);
		Xe_SetShader(xe, SHADER_TYPE_VERTEX, g_pVertexShaderPost, 0);
		//
		Xe_SetTexture(xe, 0, pPostRenderSurface);
		//Xe_SetTexture(xe, 0, GetFmvSurf());
		//Xe_SetTexture(xe,0,pRenderSurface);
		//
		Xe_SetStreamSource(xe, 0, pVbPost, 0, sizeof (fxaa_vb));
		Xe_SetIndices(xe, NULL);
		// draw
		Xe_DrawPrimitive(xe, XE_PRIMTYPE_TRIANGLELIST, 0, 1);
		// resolve
		//Xe_Resolve(xe);
		//Xe_Sync(xe); // wait for background render to finish !
		//Xe_InvalidateState(xe);
		// restore shader
		Xe_SetShader(xe, SHADER_TYPE_VERTEX, g_pVertexShader, 0);
	}
	if (debug_draw) {
		//extern unsigned char * psxVSecure;
		//xeGfx_setTextureData(GetPsxVramSurf(),psxVSecure);

		Xe_ResolveInto(xe, pPostRenderSurface, XE_SOURCE_COLOR, XE_CLEAR_DS);
		Xe_Sync(xe);

		Xe_InvalidateState(xe);

		EndPostProcess();

		Xe_SetCullMode(xe, XE_CULL_NONE);

		Xe_Clear(xe, ~0);

		// set shaders
		Xe_SetShader(xe, SHADER_TYPE_PIXEL, g_pPixelShaderPost, 0);
		Xe_SetShader(xe, SHADER_TYPE_VERTEX, g_pVertexShaderPost, 0);

		//draw psx output
		{
			//
			Xe_SetTexture(xe, 0, pPostRenderSurface);
			//
			Xe_SetStreamSource(xe, 0, pVbPost, 0, sizeof (fxaa_vb));
			Xe_SetIndices(xe, NULL);
			// draw
			Xe_DrawPrimitive(xe, XE_PRIMTYPE_TRIANGLESTRIP, 0, 2);
		}
		// draw fmv buff
		if (psxFmvSurf) {
			Xe_SetTexture(xe, 0, psxFmvSurf);
			//
			Xe_SetStreamSource(xe, 0, pVbPost, 0, sizeof (fxaa_vb));
			Xe_SetIndices(xe, NULL);
			// draw
			Xe_DrawPrimitive(xe, XE_PRIMTYPE_TRIANGLESTRIP, 4 * sizeof (fxaa_vb), 2);
		}
		// draw vram
		//        {
		//            Xe_SetTexture(xe, 0, GetPsxVramSurf());
		//            //
		//            Xe_SetStreamSource(xe, 0, pVbPost, 0, sizeof (fxaa_vb));
		//            Xe_SetIndices(xe, NULL);
		//            // draw
		//            Xe_DrawPrimitive(xe, XE_PRIMTYPE_TRIANGLELIST, 8*sizeof(fxaa_vb), 1);
		//        }


		// restore shader
		Xe_SetShader(xe, SHADER_TYPE_VERTEX, g_pVertexShader, 0);
	}

}

fxaa_vb * BuildVb(fxaa_vb *Rect, int x, int y, int h, int w) {

	{
		if (!debug_draw) {
			ScreenUv[UvTop] = ScreenUv[UvTop]*2;
			ScreenUv[UvLeft] = ScreenUv[UvLeft]*2;
		}

		// top left
		Rect[0].x = x;
		Rect[0].y = y;
		Rect[0].u = ScreenUv[UvBottom];
		Rect[0].v = ScreenUv[UvRight];

		// bottom left
		Rect[1].x = x;
		Rect[1].y = y - h - 1 / gpuRenderer.GetFramebufferHeight();
		Rect[1].u = ScreenUv[UvBottom];
		Rect[1].v = ScreenUv[UvLeft];

		// top right
		Rect[2].x = x + w - 1 / gpuRenderer.GetFramebufferWidth();
		Rect[2].y = y;
		Rect[2].u = ScreenUv[UvTop];
		Rect[2].v = ScreenUv[UvRight];

		// top right
		Rect[3].x = x + w - 1 / gpuRenderer.GetFramebufferWidth();
		Rect[3].y = y;
		Rect[3].u = ScreenUv[UvTop];
		Rect[3].v = ScreenUv[UvRight];

		int i = 0;
		for (i = 0; i < 3; i++) {
			Rect[i].z = 0.0;
			Rect[i].w = 1.0;
		}
	}

	return Rect;
}

void GpuRenderer::InitPostProcess() {
	static const struct XenosVBFFormat vbf = {
		2,
		{
			{XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4},
			{XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
		}
	};

	// create shaders
	g_pPixelShaderPost = Xe_LoadShaderFromMemory(xe, (void*) g_xps_ps_fxaa);
	Xe_InstantiateShader(xe, g_pPixelShaderPost, 0);

	g_pVertexShaderPost = Xe_LoadShaderFromMemory(xe, (void*) g_xvs_vs_fxaa);
	Xe_InstantiateShader(xe, g_pVertexShaderPost, 0);

	Xe_ShaderApplyVFetchPatches(xe, g_pVertexShaderPost, 0, &vbf);

	printf("g_pPixelShaderPost = %p\r\n", g_pPixelShaderPost);
	printf("g_pVertexShaderPost = %p\r\n", g_pVertexShaderPost);

	// create render texture - all data will be rendering into
	pPostRenderSurface = Xe_CreateTexture(xe, pRenderSurface->width, pRenderSurface->height, 0, XE_FMT_8888 | XE_FMT_ARGB, 1);
	pPostRenderSurface->use_filtering = 1;
	//Xe_SetRenderTarget(xe, pPostRenderSurface);

	pPsx = pPostRenderSurface;

	// create post vb
	float x = -1.0f;
	float y = 1.0f;
	float w = 4.0f;
	float h = 4.0f;
	/*
		float w = 2.0f;
		float h = 2.0f;
	 */

	pVbPost = Xe_CreateVertexBuffer(xe, 16 * sizeof (fxaa_vb));
	fxaa_vb *Rect = (fxaa_vb *) Xe_VB_Lock(xe, pVbPost, 0, 16 * sizeof (fxaa_vb), XE_LOCK_WRITE);

	// post
	if (debug_draw) {
		// 1/2 screen
		BuildVb(&Rect[0], -1, 1, 1, 1);
		BuildVb(&Rect[4], 0, 0, 1, 1);
		BuildVb(&Rect[8], 0, -1, 1, 1);
	} else {
		Rect = BuildVb(Rect, x, y, w, h);
	}

	Xe_VB_Unlock(xe, pVbPost);

	BeginPostProcess();
}

void GpuRenderer::UpdatesStates() {
	if (m_RenderStates.surface) {
		m_RenderStates.surface->use_filtering = XE_TEXF_POINT;
		if (postprocessenabled)
			m_RenderStates.surface->use_filtering = XE_TEXF_LINEAR;
	}
	Xe_SetTexture(xe, 0, m_RenderStates.surface);

	if (m_RenderStates.blending_enabled) {
		Xe_SetBlendControl(xe,
			m_RenderStates.blend_src, m_RenderStates.blend_op, m_RenderStates.blend_dst,
			m_RenderStates.blend_src, m_RenderStates.blend_op, m_RenderStates.blend_dst);
	} else {
		Xe_SetBlendControl(xe,
			XE_BLEND_ONE, XE_BLENDOP_ADD, XE_BLEND_ZERO,
			XE_BLEND_ONE, XE_BLENDOP_ADD, XE_BLEND_ZERO);
	}

	Xe_SetAlphaFunc(xe, m_RenderStates.alpha_test_func);
	Xe_SetAlphaRef(xe, m_RenderStates.alpha_test_ref);
	Xe_SetAlphaTestEnable(xe, m_RenderStates.alpha_test_enable);

	Xe_SetZEnable(xe, m_RenderStates.z_enable);
	Xe_SetZWrite(xe, m_RenderStates.z_write);
	Xe_SetZFunc(xe, m_RenderStates.z_func);

	//nw
/*	Xe_SetStencilEnable(xe, 1);
	Xe_SetStencilFunc(xe, 3, XE_CMP_ALWAYS);
	Xe_SetStencilWriteMask(xe, 3, 2);
	Xe_SetStencilOp(xe, 3, -1, XE_STENCILOP_INCR, XE_STENCILOP_ZERO);*/

	if (m_RenderStates.currentPsShader)
		Xe_SetShader(xe, SHADER_TYPE_PIXEL, m_RenderStates.currentPsShader, 0);
	else
		Xe_SetShader(xe, SHADER_TYPE_PIXEL, g_pPixelShaderC, 0);

	Xe_SetScissor(xe, m_RenderStates.scissor_enable,
			m_RenderStates.scissor_left, m_RenderStates.scissor_top, m_RenderStates.scissor_right, m_RenderStates.scissor_bottom);
}

void GpuRenderer::SubmitVertices() {
	// update render states
	UpdatesStates();

	// draw
	Xe_DrawIndexedPrimitive(xe, (m_PrimType == PRIM_RECTLIST) ?XE_PRIMTYPE_RECTLIST:XE_PRIMTYPE_TRIANGLELIST, 0, 0, verticesCount(), prevIndicesCount, (indicesCount() - prevIndicesCount) / 3);
	prevIndicesCount = indicesCount();
}

void GpuRenderer::InitStates() {
	m_RenderStates.blending_enabled = 0;
	m_RenderStates.blend_src = XE_BLEND_ONE;
	m_RenderStates.blend_dst = XE_BLEND_ZERO;
	m_RenderStates.blend_op = XE_BLENDOP_ADD;

	m_RenderStates.alpha_test_enable = 0;
	m_RenderStates.alpha_test_func = XE_CMP_NOTEQUAL;
	m_RenderStates.alpha_test_ref = 0;

	m_RenderStates.cullmode = XE_CULL_NONE;

	m_RenderStates.fillmode_back = 0;
	m_RenderStates.fillmode_front = 0;

	m_RenderStates.stencil_enable = 0;
	m_RenderStates.stencil_func = 0;
	m_RenderStates.stencil_mask = 0;
	m_RenderStates.stencil_op = 0;
	m_RenderStates.stencil_ref = 0;
	m_RenderStates.stencil_writemask = 0;

	m_RenderStates.surface = NULL;

	m_RenderStates.z_enable = 0;
	m_RenderStates.z_func = 0;
	m_RenderStates.z_write = 0;

	m_RenderStates.scissor_enable = 0;
}

extern struct XenosDevice * g_pVideoDevice;

void GpuRenderer::InitXe() {

#ifndef USE_GUI
	xe = &_xe;
	Xe_Init(xe);
#else
	// g_video.cpp
	extern struct XenosDevice * g_pVideoDevice;
	xe = g_pVideoDevice;
#endif

	g_xe = xe;
	pRenderSurface = Xe_GetFramebufferSurface(xe);
	Xe_SetRenderTarget(xe, pRenderSurface);

	static const struct XenosVBFFormat vbf = {
		4,
		{
			{XE_USAGE_POSITION, 0, XE_TYPE_FLOAT3},
			{XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
			{XE_USAGE_TEXCOORD, 1, XE_TYPE_FLOAT2},
			{XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
		}
	};


	g_pPixelShaderC = Xe_LoadShaderFromMemory(xe, (void*) g_xps_psC);
	Xe_InstantiateShader(xe, g_pPixelShaderC, 0);


	g_pPixelShaderF = Xe_LoadShaderFromMemory(xe, (void*) g_xps_psF);
	Xe_InstantiateShader(xe, g_pPixelShaderF, 0);

	g_pPixelShaderG = Xe_LoadShaderFromMemory(xe, (void*) g_xps_psG);
	Xe_InstantiateShader(xe, g_pPixelShaderG, 0);

	g_pPixelShader = Xe_LoadShaderFromMemory(xe, (void*) g_xps_main);
	Xe_InstantiateShader(xe, g_pPixelShader, 0);

	g_pVertexShader = Xe_LoadShaderFromMemory(xe, (void*) g_xvs_main);
	Xe_InstantiateShader(xe, g_pVertexShader, 0);

	Xe_SetShader(xe, SHADER_TYPE_PIXEL, g_pPixelShaderG, 0);
	Xe_SetShader(xe, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

	Xe_ShaderApplyVFetchPatches(xe, g_pVertexShader, 0, &vbf);

#ifndef USE_GUI
	edram_init(xe);
#endif

	for (int i = 0; i < 60; i++) {
		Xe_Resolve(xe);
		Xe_Sync(xe);
	}

	Xe_InvalidateState(xe);
}

void GpuRenderer::Lock() {
	// VB
	Xe_SetStreamSource(xe, 0, pVb, 0, 4);
	pFirstVertex = pCurrentVertex = (verticeformats *) Xe_VB_Lock(xe, pVb, 0, MAX_VERTEX_COUNT * sizeof (verticeformats), XE_LOCK_WRITE);

	// IB
	Xe_SetIndices(xe, pIb);
	pFirstIndice = pCurrentIndice = (uint16_t *) Xe_IB_Lock(xe, pIb, 0, MAX_INDICE_COUNT * sizeof (uint16_t), XE_LOCK_WRITE);
}

void GpuRenderer::Unlock() {
	
	if (pVb->lock.start)
		Xe_VB_Unlock(xe, pVb);
	
	if (pIb->lock.start)
		Xe_IB_Unlock(xe, pIb);
}

/**
 *
 * @param s
 */
void GpuRenderer::SetTexture(struct XenosSurface * s) {
	if (s != m_RenderStates.surface) {
		SubmitVertices();
		m_RenderStates.surface = s;
	}
}

void GpuRenderer::EnableTexture() {
	if (m_RenderStates.currentPsShader != g_pPixelShaderG) {
		SubmitVertices();
		m_RenderStates.currentPsShader = g_pPixelShaderG;
	}
}

void GpuRenderer::DisableTexture() {
	if (m_RenderStates.currentPsShader != g_pPixelShaderC) {
		SubmitVertices();
		m_RenderStates.currentPsShader = g_pPixelShaderC;
	}
}

/**
 * Clear
 */
void GpuRenderer::Clear(uint32_t flags) {
	SubmitVertices();

	FinishPendingClear();
	
	Xe_Clear(xe,flags);
	Xe_Execute(xe);
	clearing = true;
}

void GpuRenderer::ClearColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	Xe_SetClearColor(xe,MAKE_COLOR4(r,g,b,a));
}

// fillmode

/**
 * blend
 */
void GpuRenderer::DisableBlend() {
	if (m_RenderStates.blending_enabled) {
		SubmitVertices();
		m_RenderStates.blending_enabled = 0;
	}
}

void GpuRenderer::EnableBlend() {
	if (!m_RenderStates.blending_enabled) {
		SubmitVertices();
		m_RenderStates.blending_enabled = 1;
	}
}

void GpuRenderer::SetBlendFunc(int src, int dst) {
	if ((m_RenderStates.blend_src != src) || (m_RenderStates.blend_dst != dst)) {
		SubmitVertices();
		m_RenderStates.blend_src = src;
		m_RenderStates.blend_dst = dst;
	}
}

void GpuRenderer::SetBlendOp(int op) {
	if (m_RenderStates.blend_op != op) {
		SubmitVertices();
		m_RenderStates.blend_op = op;
	}
}

// cull mode

/**
 * Alpha test
 */
void GpuRenderer::SetAlphaFunc(int func, float ref) {
	if ((m_RenderStates.alpha_test_func != func) || (m_RenderStates.alpha_test_ref != ref)) {
		SubmitVertices();
		m_RenderStates.alpha_test_func = func;
		m_RenderStates.alpha_test_ref = ref;
	}
}

void GpuRenderer::EnableAlphaTest() {
	if (!m_RenderStates.alpha_test_enable) {
		SubmitVertices();
		m_RenderStates.alpha_test_enable = 1;
	}
}

void GpuRenderer::DisableAlphaTest() {
	if (m_RenderStates.alpha_test_enable) {
		SubmitVertices();
		m_RenderStates.alpha_test_enable = 0;
	}
}

// stencil

// scissor

void GpuRenderer::SetScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	uint32_t top = GetFramebufferHeight()-(y + height - 1);
	uint32_t bottom = GetFramebufferHeight() - y;
	uint32_t left = x;
	uint32_t right = x + width - 1;
	if (
			m_RenderStates.scissor_left != left ||
			m_RenderStates.scissor_top != top ||
			m_RenderStates.scissor_right != right ||
			m_RenderStates.scissor_bottom != bottom

			) {
		SubmitVertices();
		m_RenderStates.scissor_left = left;
		m_RenderStates.scissor_top = top;
		m_RenderStates.scissor_right = right;
		m_RenderStates.scissor_bottom = bottom;
	}
}

void GpuRenderer::DisableScissor() {
	if (m_RenderStates.scissor_enable) {
		SubmitVertices();
		m_RenderStates.scissor_enable = 0;
	}
};

void GpuRenderer::EnableScissor() {
	if (!m_RenderStates.scissor_enable) {
		SubmitVertices();
		m_RenderStates.scissor_enable = 1;
	}
};

/**
 * z / depth
 */
void GpuRenderer::EnableDepthTest() {
	if (!m_RenderStates.z_write || !m_RenderStates.z_enable) {
		SubmitVertices();
		m_RenderStates.z_write = 1;
		m_RenderStates.z_enable = 1;
	}
}

void GpuRenderer::DisableDepthTest() {
	if (m_RenderStates.z_write || m_RenderStates.z_enable) {
		SubmitVertices();
		m_RenderStates.z_write = 0;
		m_RenderStates.z_enable = 0;
	}
}

void GpuRenderer::DepthFunc(int func) {
	if (m_RenderStates.z_func != func) {
		SubmitVertices();
		m_RenderStates.z_func = func;
	}
}

/**
 * Init
 */
void GpuRenderer::Init() {
	InitXe();
	InitStates();

	pVb = Xe_CreateVertexBuffer(xe, MAX_VERTEX_COUNT * sizeof (verticeformats));
	pIb = Xe_CreateIndexBuffer(xe, MAX_VERTEX_COUNT * sizeof (uint16_t), XE_FMT_INDEX16);

	m_RenderStates.currentPsShader = g_pPixelShaderC;
	InitPostProcess();
	Lock();
}

void GpuRenderer::Close() {

}

// view
static float screen[2] = {0, 0};
static float mwp[4][4];

void GpuRenderer::SetOrtho(float l, float r, float b, float t, float zn, float zf) {
	/*
	screen[0] = r;
	screen[1] = b;
	printf("setOrtho => %d - %d \r\n", r, b);
	Xe_SetVertexShaderConstantF(xe, 1, (float*) screen, 1);
	 * */
	/*
	// identity
	mwp[0][1] = mwp[0][2] = mwp[0][3] =
	mwp[1][0] = mwp[1][2] = mwp[1][3] =
	mwp[2][0] = mwp[2][1] = mwp[2][3] =
	mwp[3][0] = mwp[3][1] = mwp[3][2] = 0;
	mwp[0][0] = mwp[1][1] = mwp[2][2] = mwp[3][3] = 1.f;
	 */

	mwp[0][0] = 2 / (r - l);
	mwp[0][1] = 0;
	mwp[0][2] = 0;
	mwp[0][3] = 0;

	mwp[1][0] = 0;
	mwp[1][1] = 2 / (t - b);
	mwp[1][2] = 0;
	mwp[1][3] = 0;

	mwp[2][0] = 0;
	mwp[2][1] = 0;
	mwp[2][2] = 1 / (zn - zf);
	mwp[2][3] = 0;

	mwp[3][0] = (l + r) / (l - r);
	mwp[3][1] = (t + b) / (b - t);
	mwp[3][2] = zn / (zn - zf);
	mwp[3][3] = 1;

	// mul with display size


	// dump it ...
	//    printf("Dump ..\r\n");
	//    for (int i = 0; i < 4; i++) {
	//        printf("[%0.2f][%0.2f][%0.2f][%0.2f]\r\n", mwp[i][0], mwp[i][1], mwp[i][2], mwp[i][3]);
	//    }

	screen[0] = r;
	screen[1] = b;
	//    printf("setOrtho => %f - %f \r\n", r, b);

	Xe_SetVertexShaderConstantF(xe, 0, (float*) mwp, 4);
	Xe_SetVertexShaderConstantF(xe, 1, (float*) screen, 1);
}

void GpuRenderer::SetViewPort(int left, int top, int right, int bottom) {
	/*
		TR

		// identity
		printf("SetViewPort lt => %d - %d \r\n", left, top);
		printf("SetViewPort rb=> %d - %d \r\n", right, bottom);

		printf("Dump ..\r\n");
		for (int i = 0; i < 4; i++) {
			printf("[%0.2f][%0.2f][%0.2f][%0.2f]\r\n", mwp[i][0], mwp[i][1], mwp[i][2], mwp[i][3]);
		}
	 */
	//Xe_SetVertexShaderConstantF(xe, 0, (float*) mwp, 4);
}

uint32_t GpuRenderer::GetFramebufferWidth() {
	return Xe_GetFramebufferSurface(xe)->width;
}

uint32_t GpuRenderer::GetFramebufferHeight() {
	return Xe_GetFramebufferSurface(xe)->height;
}

/**
 * DoRender
 */
void GpuRenderer::Render() {
	// Submit last batch of vertices
    SubmitVertices();

	Unlock();

	FinishPendingClear();
	
	// Resolve in temporary surface
	RenderPostProcess();

	rendering = true;

	Xe_ResolveInto(xe, Xe_GetFramebufferSurface(xe), XE_SOURCE_COLOR, XE_CLEAR_DS);
	
#if 1
	Xe_Execute(xe); // start background render !
#else	
	FinishPendingRender();
#endif	
}

/**
 * ResetRender
 */
void GpuRenderer::FinishPendingRender() {
	if (!rendering)
		return;
	
	Xe_Sync(xe); // wait for background render to finish !
	
	rendering = false;

	Xe_InvalidateState(xe);

	doScreenCapture();

	BeginPostProcess();

	// relock
	Lock();
	Xe_SetCullMode(xe, XE_CULL_NONE);
	Xe_SetVertexShaderConstantF(xe, 0, (float*) mwp, 4);
	Xe_SetVertexShaderConstantF(xe, 1, (float*) screen, 1);

	// restore shader
	Xe_SetShader(xe, SHADER_TYPE_VERTEX, g_pVertexShader, 0);
	// Wireframe
	//Xe_SetFillMode(xe,0x25,0x25);

	prevIndicesCount = 0;

	nb_indices = 0;
	nb_vertices = 0;
}

void GpuRenderer::FinishPendingClear() {
	if (clearing)
	{
		Xe_Execute(xe); // wait for background clear to finish !
		Xe_VBReclaim(xe);
		clearing = false;
	}
}

/**
 * Add to Vb
 */
int GpuRenderer::verticesCount() {
	return nb_vertices;
}

int GpuRenderer::indicesCount() {
	return nb_indices;
};

void GpuRenderer::NextVertice() {
	nb_vertices++;
	pCurrentVertex++;
}

void GpuRenderer::NextIndice() {
	nb_indices++;
	pCurrentIndice++;
}

void GpuRenderer::primBegin(int primType) {
	if(m_PrimType!=primType){
		SubmitVertices();
		m_PrimType = primType;
	}
	
	prevVerticesCount = verticesCount();
};

void GpuRenderer::primEnd() {
	// do ib
	switch (m_PrimType) {
		case PRIM_TRIANGLE:
		{
			for (int i = 0; i < 3; i++) {
				pCurrentIndice[0] = prevVerticesCount + i;
				NextIndice();
			}
			break;
		}
		case PRIM_TRIANGLE_STRIP:
		{
			for (int i = 0; i < 6; i++) {
				pCurrentIndice[0] = prevVerticesCount + indices_strip[i];
				NextIndice();
			}
			break;
		}
		case PRIM_QUAD:
		{
			for (int i = 0; i < 6; i++) {
				pCurrentIndice[0] = prevVerticesCount + indices_quad[i];
				NextIndice();
			}
			break;
		}
		case PRIM_RECTLIST:
		{
			for (int i = 0; i < 3; i++) {
				pCurrentIndice[0] = prevVerticesCount + i;
				NextIndice();
			}
			break;
		}
	}
};

void GpuRenderer::primTexCoord(float * st) {
	textcoord_u = st[0];
	textcoord_v = st[1];
};

void GpuRenderer::primVertex(float * v) {
	pCurrentVertex[0].x = v[0];
	pCurrentVertex[0].y = v[1];
	pCurrentVertex[0].z = v[2];
	pCurrentVertex[0].u = textcoord_u;
	pCurrentVertex[0].v = textcoord_v;
	pCurrentVertex[0].color = m_primColor;
	NextVertice();
};

void GpuRenderer::primColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	m_primColor = MAKE_COLOR4(r,g,b,a);
};

static int n_texture = 0;
static uint64_t allocated_texture_size = 0;

void GpuRenderer::DestroyTexture(XenosSurface *surf) {
	if (surf) {
		allocated_texture_size -= surf->hpitch * surf->wpitch;
		n_texture--;
	}

	Xe_DestroyTexture(xe, surf);
};

XenosSurface * GpuRenderer::CreateTexture(unsigned int width, unsigned int height, int format) {
	XenosSurface * surf = Xe_CreateTexture(xe, width, height, 0, format, 0);

	allocated_texture_size += surf->hpitch * surf->wpitch;
	n_texture++;

	return surf;
}

/**
 * Textures
 */
void XeTexSubImage(struct XenosSurface * surf, int srcbpp, int dstbpp, int xoffset, int yoffset, int width, int height, const void * buffer) {

	//    printf("xoffset : %d\r\n",xoffset);
	//    printf("yoffset : %d\r\n",yoffset);
	//    printf("width : %d\r\n",width);
	//    printf("height : %d\r\n",height);
	//    printf("buffer : %p\r\n",buffer);

	if (surf) {
		//copy data
		//printf("xeGfx_setTextureData\n");
		uint8_t * surfbuf = (uint8_t*) Xe_Surface_LockRect(xe, surf, 0, 0, 0, 0, XE_LOCK_WRITE);
		uint8_t * srcdata = (uint8_t*) buffer;
		uint8_t * dstdata = surfbuf;
		int srcbytes = srcbpp;
		int dstbytes = dstbpp;
		int y, x;

		int pitch = (surf->wpitch);
		int offset = 0;

		for (y = yoffset; y < (yoffset + height); y++) {
			offset = (y * pitch)+(xoffset * dstbytes);

			dstdata = surfbuf + offset; // ok
			//dstdata = surfbuf + (y*pitch)+(offset);// ok
			for (x = xoffset; x < (xoffset + width); x++) {
				if (srcbpp == 4 && dstbytes == 4) {
					dstdata[0] = srcdata[0];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[2];
					dstdata[3] = srcdata[3];

					srcdata += srcbytes;
					dstdata += dstbytes;
				}
			}
		}

		Xe_Surface_Unlock(xe, surf);
	}
}

void xeGfx_setTextureData(void * tex, void * buffer) {
	//printf("xeGfx_setTextureData\n");
	struct XenosSurface * surf = (struct XenosSurface *) tex;
	if (surf) {
		//copy data
		// printf("xeGfx_setTextureData\n");
		uint8_t * surfbuf = (uint8_t*) Xe_Surface_LockRect(xe, surf, 0, 0, 0, 0, XE_LOCK_WRITE);
		uint8_t * srcdata = (uint8_t*) buffer;
		uint8_t * dstdata = surfbuf;
		int srcbytes = 4;
		int dstbytes = 4;
		int y, x;

		int pitch = surf->wpitch;

		for (y = 0; y < (surf->height); y++) {
			dstdata = surfbuf + (y)*(pitch); // ok
			for (x = 0; x < (surf->width); x++) {

				dstdata[0] = srcdata[0];
				dstdata[1] = srcdata[1];
				dstdata[2] = srcdata[2];
				dstdata[3] = srcdata[3];

				srcdata += srcbytes;
				dstdata += dstbytes;
			}
		}

		Xe_Surface_Unlock(xe, surf);
	}
}

void * GpuRenderer::TextureLock(GpuTex *surf) {
	return Xe_Surface_LockRect(xe, surf, 0, 0, 0, 0, XE_LOCK_WRITE);
};

void GpuRenderer::TextureUnlock(GpuTex *surf) {
	Xe_Surface_Unlock(xe, surf);
};

// return last post processess texture

GpuTex * GpuRenderer::GetFB() {
	return Xe_GetFramebufferSurface(xe);
}

void XeTexCopyImage(struct XenosSurface * surf, int srcbpp, int dstbpp, int xoffset, int yoffset, int width, int height, void * dest) {
	if (surf) {
		//copy data
		//printf("xeGfx_setTextureData\n");
		uint8_t * surfbuf = (uint8_t*) Xe_Surface_LockRect(xe, surf, 0, 0, 0, 0, XE_LOCK_WRITE);
		uint8_t * srcdata = (uint8_t*) surfbuf;
		uint8_t * dstdata = (uint8_t*) dest;
		int srcbytes = srcbpp;
		int dstbytes = dstbpp;
		int y, x;

		int pitch = (surf->wpitch);
		int offset = 0;

		for (y = yoffset; y < (yoffset + height); y++) {
			offset = (y * pitch)+(xoffset * srcbytes);

			srcdata = surfbuf + offset; // ok
			//dstdata = surfbuf + (y*pitch)+(offset);// ok
			for (x = xoffset; x < (xoffset + width); x++) {
				if (srcbpp == 4 && dstbytes == 4) {
					dstdata[0] = srcdata[0];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[2];
					dstdata[3] = srcdata[3];

					srcdata += srcbytes;
					dstdata += dstbytes;
				}
			}
		}

		Xe_Surface_Unlock(xe, surf);
	}
}

void xeReadPixels(struct XenosSurface * surf, int x, int y, int width, int height, void *pixels) {
	int row;
	int col;
	unsigned char *srcdata;
	unsigned char *dstdata = (unsigned char *) pixels;

	srcdata = (unsigned char *) Xe_Surface_LockRect(xe, surf, 0, 0, 0, 0, XE_LOCK_WRITE);

	// invert the copied image
	for (row = (y + height - 1); row >= y; row--) {
		for (col = x; col < (x + width); col++) {
			int srcpos = row * width + col;

			// swap to BGR because Quake is going to swap it back
			dstdata[2] = srcdata[srcpos * 3 + 0];
			dstdata[1] = srcdata[srcpos * 3 + 1];
			dstdata[0] = srcdata[srcpos * 3 + 2];

			dstdata += 3;
		}
	}

	Xe_Surface_Unlock(xe, surf);
}