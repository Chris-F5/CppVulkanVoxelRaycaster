#pragma once

void loadPlyVoxModel(
    char *filename,
    unsigned int *paletteSize,
    unsigned char **palette,
    unsigned int *modelWidth,
    unsigned int *modelHeight,
    unsigned int *modelDepth,
    unsigned char **model);
