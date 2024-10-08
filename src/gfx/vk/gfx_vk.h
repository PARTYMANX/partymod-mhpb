#ifndef _GFX_VK_H_
#define _GFX_VK_H_

// contains internal data for the vulkan driver
// contains common structs (ie renderer) for the vulkan driver

#include <stdint.h>

#include <vulkan/vulkan.h>

#define FLUSH_ALL
#define GFX_COLOR_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define GFX_DEPTH_FORMAT VK_FORMAT_D24_UNORM_S8_UINT	// used by MHPB
//#define GFX_DEPTH_FORMAT VK_FORMAT_D16_UNORM	// used by THPS2
//#define SHADERS_FROM_FILE

/*
	STRUCT
*/

typedef struct partyRenderer partyRenderer;

typedef struct {
	float x;
	float y;
	float z;
	float w;
	float u;
	float v;
	uint32_t color;
	int16_t texture;
	int16_t flags;
} renderVertex;

typedef struct {
	float x;
	float y;
	float u;
	float v;
} scalerVertex;

typedef struct {
	float width;
	float height;
} imageInfo;

/*
	FUNC
*/

uint8_t CreateVKRenderer(void *windowHandle, partyRenderer **dst);
void updateRenderer(partyRenderer *renderer);

void startRender(partyRenderer *renderer, uint32_t clearColor);
void finishRender(partyRenderer *renderer);
void drawVertices(partyRenderer *renderer, renderVertex *vertices, uint32_t vertex_count, uint8_t clamp);
//void drawTriangleFan(partyRenderer *renderer, renderVertex *vertices, uint32_t vertex_count);
void drawLines(partyRenderer *renderer, renderVertex *vertices, uint32_t vertex_count);
void setViewport(partyRenderer *renderer, float x, float y, float width, float height);
void setScissor(partyRenderer *renderer, float x, float y, float width, float height);
void setDepthState(partyRenderer *renderer, uint8_t test, uint8_t write);
void setBlendState(partyRenderer *renderer, uint32_t blendState);
void setRenderResolution(partyRenderer *renderer, uint32_t width, uint32_t height, float aspectRatio);
void setTextureFilter(partyRenderer *renderer, uint8_t filter);
uint32_t createTextureEntry(partyRenderer *renderer, uint32_t width, uint32_t height);
void updateTextureEntry(partyRenderer *renderer, uint32_t idx, uint32_t width, uint32_t height, void *data);
void destroyTextureEntry(partyRenderer *renderer, uint32_t idx);
int getTextureCount(partyRenderer *renderer);
void clearAllTextures(partyRenderer *renderer);	// DANGER: this will cause dangling pointers!!!
void renderImageFrame(partyRenderer *renderer, uint32_t texIdx);


#endif _GFX_VK_H_