#pragma once

#include <stddef.h>

#include "memory_pool.hpp"

const int VOX_BLOCK_SCALE = 16;
const int VOX_BLOCK_POINT_COUNT = VOX_BLOCK_SCALE * VOX_BLOCK_SCALE * VOX_BLOCK_SCALE;

struct VoxBlock{
    unsigned char voxels[VOX_BLOCK_POINT_COUNT];
};

struct Material{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

struct Palette{ 
    Material mats[256];
};

struct VoxObject{
    uint32_t paletteIndex;
    uint32_t blockWidth;
    uint32_t blockHeight;
    uint32_t blockDepth;
    uint32_t *blockIndices;
};

void loadPlyVoxObject(
    char *filename,
    MemPool<VoxBlock> voxBlocks,
    MemPool<Palette> palettes,
    VoxObject *voxObject);
