#include "vox_object.hpp"

#include <stdio.h>
#include <limits.h>
#include <iostream>

char END_HEADER[] = "end_header\n";

size_t voxBlockIndex(unsigned int x, unsigned int y, unsigned int z){
    return x + y * VOX_BLOCK_SCALE + z * VOX_BLOCK_SCALE * VOX_BLOCK_SCALE;
}

long int skipPlyHeader(FILE* file){
    int endHeaderIndex = 0;
    int c;
    while(endHeaderIndex < sizeof(END_HEADER) - 1){
        c = fgetc(file);
        if(c == EOF)
            throw std::runtime_error("voxel object file does not end header");
        if(c == END_HEADER[endHeaderIndex])
            endHeaderIndex++;
    }
    return ftell(file);
}

void loadPlyVoxObject (
    char *filename,
    MemPool<VoxBlock> voxBlocks,
    MemPool<Palette> palettes,
    VoxObject *voxObject)
{
    FILE *file;
    if((file = fopen(filename, "r")) == NULL){
        throw std::runtime_error("cant open voxel object file");
    }

    long int headerEndLocation = skipPlyHeader(file);

    int minX, minY, minZ;
    int maxX, maxY, maxZ;
    minX = minY = minZ = INT_MAX;
    maxX = maxY = maxZ = INT_MIN;

    int x, y, z, r, g, b = 0;
    while(fscanf(file, "%d %d %d %d %d %d\n", &x, &y, &z, &r, &g, &b) != EOF){
        if(x < minX)
            minX = x;
        if(y < minY)
            minY = y;
        if(z < minZ)
            minZ = z;
        if(x > maxX)
            maxX = x;
        if(y > maxY)
            maxY = y;
        if(z > maxZ)
            maxZ = z;
    }

    unsigned int modelWidth = maxX - minX + 1;
    unsigned int modelHeight = maxY - minY + 1;
    unsigned int modelDepth = maxZ - minZ + 1;

    voxObject->paletteIndex = palettes.allocateBlock();
    voxObject->blockWidth =  (modelWidth + VOX_BLOCK_SCALE - 1) / VOX_BLOCK_SCALE;
    voxObject->blockHeight = (modelHeight + VOX_BLOCK_SCALE - 1) / VOX_BLOCK_SCALE;
    voxObject->blockDepth =  (modelDepth + VOX_BLOCK_SCALE - 1) / VOX_BLOCK_SCALE;
    voxObject->blockIndices = (unsigned int *)calloc(
        voxObject->blockWidth * voxObject->blockHeight * voxObject->blockDepth,
        sizeof(unsigned int));

    Palette *palette = palettes.getBlock(voxObject->paletteIndex);

    fseek(file, headerEndLocation, SEEK_SET);

    unsigned int matCount = 0;
    while(fscanf(file, "%d %d %d %d %d %d\n", &x, &y, &z, &r, &g, &b) != EOF){
        Material mat = Material{(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
        bool foundPaletteIndex = false;
        unsigned char matIndex;
        for(int i = 0; i < matCount; i++)
            if( palette->mats[i].r == mat.r &&
                palette->mats[i].g == mat.g &&
                palette->mats[i].b == mat.b){
                    foundPaletteIndex = true;
                    matIndex = i;
            }
        if(!foundPaletteIndex){
            if(matCount == 255)
                throw std::runtime_error("ply voxel object has too many materials to fit in palette");
            palette->mats[matCount] = mat;
            matIndex = matCount++;
        }

        x -= minX;
        y -= minY;
        z -= minZ;

        unsigned int blockX = x / VOX_BLOCK_SCALE;
        unsigned int blockY = y  / VOX_BLOCK_SCALE;
        unsigned int blockZ = z  / VOX_BLOCK_SCALE;

        unsigned int blockLocalX = x % VOX_BLOCK_SCALE;
        unsigned int blockLocalY = y % VOX_BLOCK_SCALE;
        unsigned int blockLocalZ = z % VOX_BLOCK_SCALE;

        size_t blockObjectIndex = 
            blockX + blockY * voxObject->blockWidth + blockZ * voxObject->blockWidth * voxObject->blockHeight;
        
        if(voxObject->blockIndices[blockObjectIndex] == 0)
            voxObject->blockIndices[blockObjectIndex] = voxBlocks.allocateBlock() + 1;
        
        VoxBlock *block = voxBlocks.getBlock(voxObject->blockIndices[blockObjectIndex] - 1);
        block->voxels[voxBlockIndex(blockLocalX, blockLocalY, blockLocalZ)] = matIndex + 1;
    }
    fclose(file);
}
