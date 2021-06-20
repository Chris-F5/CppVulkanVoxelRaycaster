// !!! This file is temporary. If there are any bugs in the program, they are in here.

#include "octree_generator.hpp"

#include <iostream>
#include <stdio.h>
#include <math.h>

const uint NODE_TYPE_PARENT = 0;
const uint NODE_TYPE_COLORED = 1;
const uint NODE_TYPE_EMPTY = 2;

std::vector<int> readPointmapFile(std::string filename)
{
    std::vector<int> data;
    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL)
    {
        std::runtime_error("pointmap file does not exist");
    }
    else
    {
        bool inHeader = true;
        char c;
        std::string current = "";
        while ((c = fgetc(file)) != EOF)
        {
            if (c == ' ' || c == '\n' || c == '\r')
            {
                if (current.length() != 0)
                {
                    if (!inHeader)
                    {
                        data.push_back(std::stoi(current));
                    }
                    else if (current.compare("end_header") == 0)
                    {
                        inHeader = false;
                    }
                }
                current.clear();
            }
            else
                current.push_back(c);
        }
    }
    return data;
}

uint normalizeRawPointmapData(std::vector<int> *data)
{
    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();
    int minZ = std::numeric_limits<int>::max();
    int maxZ = std::numeric_limits<int>::min();
    for (int i = 0; i < data->size(); i += 6)
    {
        int x = data->at(i);
        int y = data->at(i + 1);
        int z = data->at(i + 2);
        if (x < minX)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (y < minY)
            minY = y;
        if (y > maxY)
            maxY = y;
        if (z < minZ)
            minZ = z;
        if (z > maxZ)
            maxZ = z;
    }
    int xDiff = maxX - minX;
    int yDiff = maxY - minY;
    int zDiff = maxZ - minZ;
    int maxDiff;
    if (xDiff < yDiff && xDiff < zDiff)
        maxDiff = xDiff;
    else if (yDiff > zDiff)
        maxDiff = yDiff;
    else
        maxDiff = zDiff;

    for (int i = 0; i < data->size(); i += 6)
    {
        data->at(i) -= minX;
        data->at(i + 1) -= minY;
        data->at(i + 2) -= minZ;
    }

    uint size = 2;
    uint depth = 2;
    while (size < maxDiff + 1)
    {
        size *= 2;
        depth += 1;
    }
    return depth;
}

void insertIntoOctree(
    std::vector<uint> *octree,
    uint depth,
    uint x,
    uint y,
    uint z,
    uint r,
    uint g,
    uint b)
{
    uint index = 0;
    uint currentSize = std::pow(2, depth - 1);
    uint currentXPos = 0;
    uint currentYPos = 0;
    uint currentZPos = 0;
    for (int d = 0; d < depth - 1; d++)
    {
        uint childIndex = 0;
        if (x >= currentXPos + currentSize / 2)
        {
            childIndex += 1;
            currentXPos += currentSize / 2;
        }
        if (y >= currentYPos + currentSize / 2)
        {
            childIndex += 2;
            currentYPos += currentSize / 2;
        }
        if (z >= currentZPos + currentSize / 2)
        {
            childIndex += 4;
            currentZPos += currentSize / 2;
        }
        if (octree->at(index) != NODE_TYPE_PARENT)
        {
            octree->at(index) = NODE_TYPE_PARENT;
            octree->at(index + 1) = 0;
            octree->at(index + 2) = 0;
            octree->at(index + 3) = 0;
            octree->at(index + 4) = octree->size();
            for (int i = 0; i < 8; i++)
            {
                octree->push_back(NODE_TYPE_EMPTY);
                octree->push_back(0);
                octree->push_back(0);
                octree->push_back(0);
                octree->push_back(0);
            }
        }
        index = octree->at(index + 4) + childIndex * 5;
        currentSize /= 2;
    }
    octree->at(index) = NODE_TYPE_COLORED;
    octree->at(index + 1) = r;
    octree->at(index + 2) = g;
    octree->at(index + 3) = b;
    octree->at(index + 4) = 0;
}

std::vector<uint32_t> getOctreeFromFile(std::string filename)
{
    std::vector<int> raw = readPointmapFile(filename);
    uint depth = normalizeRawPointmapData(&raw);
    std::vector<uint32_t> octree{NODE_TYPE_EMPTY, 000, 000, 000, 0};
    for (int i = 0; i < raw.size(); i += 6)
    {
        uint x = raw[i];
        uint y = raw[i + 1];
        uint z = raw[i + 2];
        uint r = raw[i + 3];
        uint g = raw[i + 4];
        uint b = raw[i + 5];
        //std::cout << "== " << x << " " << y << " " << z << "\n";
        insertIntoOctree(&octree, depth, x, y, z, r, g, b);
    }
    std::cout << depth << "\n";
    return octree;
}
