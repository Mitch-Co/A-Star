#ifndef ALGOS_H
#define ALGOS_H

#include <stdint.h>
#include <stdbool.h>
#include "bmp.h"

typedef struct GRAPH_NODE {
    struct GRAPH_NODE* up;
    uint16_t upCost;
    struct GRAPH_NODE* down;
    uint16_t downCost;
    struct GRAPH_NODE* left;
    uint16_t leftCost;
    struct GRAPH_NODE* right;
    uint16_t rightCost;

    bool visited;
    uint32_t cost;
    struct GRAPH_NODE* from;

} NODE;

typedef struct GRAPH_STRUCT {
    NODE* start;
    NODE* end;

    // NOTE: size includes start and end nodes
    uint32_t size;
} GRAPH;

GRAPH* graphFromBMP(BMP* toConvert);


#endif