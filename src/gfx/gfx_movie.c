#include <stdint.h>
#include <stdio.h>

#include <patch.h>

#include <gfx/vk/gfx_vk.h>
#include <gfx/gfx_global.h>

//void (__stdcall **BinkClose)(void *) = baseAddr + 0x00044188;
//void (__stdcall **BinkDoFrame)(void *) = baseAddr + 0x0004418c;
//void (__stdcall **BinkNextFrame)(void *) = baseAddr + 0x00044198;
//void (__stdcall **BinkCopyToBuffer)(void *, void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) = baseAddr + 0x00044190;
//void (__stdcall **BinkGoto)(void *, int, int) = baseAddr + 0x00044194;

//void **pBink = baseAddr + 0x0018cb8c;
//uint8_t *BinkHasVideo = baseAddr + 0x0018cb80;
//uint8_t *BinkIsPlaying = baseAddr + 0x0018cb81;
//uint32_t *BinkWidth = baseAddr + 0x0018c948;
//uint32_t *BinkHeight = baseAddr + 0x0018c94c;

//struct texture *binkTex = baseAddr + 0x0018ca38;
uint32_t binkTexIdx = -1;
uint32_t *binkBuffer = NULL;

uint32_t __cdecl createBinkSurface() {
	//uint32_t *gShellMode = 0x006a35b4;
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	uint32_t *BinkWidth = baseAddr + 0x0018c948;
	uint32_t *BinkHeight = baseAddr + 0x0018c94c;
	struct texture *binkTex = baseAddr + 0x0018ca38;

	binkBuffer = malloc(sizeof(uint32_t) * *BinkWidth * *BinkHeight);

	binkTexIdx = createTextureEntry(renderer, *BinkWidth, *BinkHeight);

	binkTex->idx = binkTexIdx;
	binkTex->flags = 0x01000000;
	binkTex->width = *BinkWidth;
	binkTex->height = *BinkHeight;
	binkTex->buf_width = *BinkWidth;
	binkTex->buf_height = *BinkHeight;

	struct texture **pBinkTex = baseAddr + 0x0018c92c;
	float *maxWidth = baseAddr + 0x0018ca78;
	float *maxHeight = baseAddr + 0x0018ca7c;

	*pBinkTex = binkTex;
	*maxWidth = 1.0f;
	*maxHeight = 1.0f;

	return 1;
}

void __cdecl stopBinkMovie() {
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	void (__stdcall **BinkClose)(void *) = baseAddr + 0x00044188;

	void **pBink = baseAddr + 0x0018cb8c;
	uint8_t *BinkIsPlaying = baseAddr + 0x0018cb81;

	if (*pBink) {
		(*BinkClose)(*pBink);
		*pBink = NULL;

		destroyTextureEntry(renderer, binkTexIdx);

		free(binkBuffer);
		binkBuffer = NULL;
	}

	*BinkIsPlaying = 0;
}

uint32_t __cdecl advanceBinkMovie() {
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	void (__stdcall **BinkDoFrame)(void *) = baseAddr + 0x0004418c;
	void (__stdcall **BinkNextFrame)(void *) = baseAddr + 0x00044198;
	void (__stdcall **BinkCopyToBuffer)(void *, void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) = baseAddr + 0x00044190;
	void (__stdcall **BinkGoto)(void *, int, int) = baseAddr + 0x00044194;

	void **pBink = baseAddr + 0x0018cb8c;
	uint8_t *BinkHasVideo = baseAddr + 0x0018cb80;
	uint8_t *BinkIsPlaying = baseAddr + 0x0018cb81;
	uint32_t *BinkWidth = baseAddr + 0x0018c948;
	uint32_t *BinkHeight = baseAddr + 0x0018c94c;

	//uint32_t *gShellMode = 0x006a35b4;

	if (!*pBink) {
		*BinkIsPlaying = 0;
		return 0;
	}

	(*BinkDoFrame)(*pBink);

	(*BinkCopyToBuffer)(*pBink, binkBuffer, sizeof(uint32_t) * *BinkWidth, *BinkHeight, 0, 0, 6);	// flags copy all and 32bit color

	for (int i = 0; i < *BinkWidth * *BinkHeight; i++) {
		// fixes opacity.  dumb.
		binkBuffer[i] |= 0xff000000;
	}

	updateTextureEntry(renderer, binkTexIdx, *BinkWidth, *BinkHeight, binkBuffer);
	if (*BinkHasVideo && !isMinimized) {
		renderImageFrame(renderer, binkTexIdx);
	}

	uint8_t *pBink8 = *pBink;
	if (*((uint32_t *)(pBink8 + 0xc)) != *((uint32_t *)(pBink8 + 0x8))) {
		(*BinkNextFrame)(*pBink);
		return 1;
	}

	if (!*BinkIsPlaying) {
		return 0;
	}

	(*BinkGoto)(*pBink, 1, 0);
	return 1;
}

void __cdecl drawBinkSurface() {
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	uint32_t *BinkWidth = baseAddr + 0x0018c948;
	uint32_t *BinkHeight = baseAddr + 0x0018c94c;

	updateTextureEntry(renderer, binkTexIdx, *BinkWidth, *BinkHeight, binkBuffer);
	renderImageFrame(renderer, binkTexIdx);
}

void __cdecl setupBinkRender() {
}

void updateMovieTexture() {
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	void **pBink = baseAddr + 0x0018cb8c;

	uint32_t *BinkWidth = baseAddr + 0x0018c948;
	uint32_t *BinkHeight = baseAddr + 0x0018c94c;

	if (*pBink) {
		updateTextureEntry(renderer, binkTexIdx, *BinkWidth, *BinkHeight, binkBuffer);
	}
}

void installMoviePatches(int module, uint32_t baseAddr) {
	//patchJmp(0x004e3760, startBinkMovie);
	//patchJmp(0x004e3f70, stopBinkMovie);
	patchJmp(baseAddr + 0x000222a0, advanceBinkMovie);

	patchJmp(baseAddr + 0x00021da0, createBinkSurface);
	//patchJmp(0x004e2c50, drawBinkSurface);
	//patchJmp(0x004e28f0, setupBinkRender);

	//patchByte(0x004e319c, 0xeb);	// remove d3d use from movie player
}