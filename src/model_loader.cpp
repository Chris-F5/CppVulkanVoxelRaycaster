#include "model_loader.hpp"

#include <stdio.h>
#include <limits.h>
#include <iostream>

char END_HEADER[] = "end_header\n";

long int skipPlyHeader(FILE* file){
    int endHeaderIndex = 0;
    int c;
    while(endHeaderIndex < sizeof(END_HEADER) - 1){
        c = fgetc(file);
        if(c == EOF)
            throw std::runtime_error("voxel model file does not end header");
        if(c == END_HEADER[endHeaderIndex])
            endHeaderIndex++;
    }
    return ftell(file);
}

void loadPlyVoxModel(
    char *filename,
    unsigned int *paletteSizeOut,
    unsigned char **paletteOut,
    unsigned int *modelWidthOut,
    unsigned int *modelHeightOut,
    unsigned int *modelDepthOut,
    unsigned char **modelOut)
{
    FILE *file;
    if((file = fopen(filename, "r")) == NULL){
        throw std::runtime_error("cant open voxel model file");
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

    unsigned char *model = (unsigned char *)calloc(modelWidth * modelHeight * modelDepth, sizeof(unsigned char));
    unsigned char *palette = (unsigned char *)malloc(256 * 4);
    fseek(file, headerEndLocation, SEEK_SET);

    unsigned int paletteSize = 0;
    while(fscanf(file, "%d %d %d %d %d %d\n", &x, &y, &z, &r, &g, &b) != EOF){
        bool foundPaletteIndex = false;
        unsigned char paletteIndex;
        for(int i = 0; i < paletteSize; i++){
            if( palette[i * 4 + 0] == r &&
                palette[i * 4 + 1] == g &&
                palette[i * 4 + 2] == b){
                    foundPaletteIndex = true;
                    paletteIndex = i;
            }
        }
        if(!foundPaletteIndex){
            if(paletteSize == 255)
                throw std::runtime_error("ply voxel model has too many colors to fit in palette");
            palette[paletteSize * 4 + 0] = r;
            palette[paletteSize * 4 + 1] = g;
            palette[paletteSize * 4 + 2] = b;
            palette[paletteSize * 4 + 3] = 255;
            paletteIndex = paletteSize++;
        }

        x -= minX;
        y -= minY;
        z -= minZ;

        model[x + y * modelWidth + z * modelWidth * modelHeight] = paletteIndex + 1;
    }
    fclose(file);

    *paletteSizeOut = paletteSize;
    *paletteOut = palette;
    *modelWidthOut = modelWidth;
    *modelHeightOut = modelHeight;
    *modelDepthOut = modelDepth;
    *modelOut = model;
}
