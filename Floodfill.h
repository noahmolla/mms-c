#pragma once

#include "API.h"

#define FLOODFILL_MAX_WIDTH 16
#define FLOODFILL_MAX_HEIGHT 16
#define FLOODFILL_MAX_GOALS 4

typedef struct {
    int x;
    int y;
} FloodfillCell;

void Floodfill_init(void);
void Floodfill_setGoals(const FloodfillCell* goals, int goalCount);
void Floodfill_markWall(FloodfillCell cell, API_Direction direction, int present);
void Floodfill_recalculate(void);
int Floodfill_distanceAt(FloodfillCell cell);
int Floodfill_canMove(FloodfillCell cell, API_Direction direction);
FloodfillCell Floodfill_neighbor(FloodfillCell cell, API_Direction direction);
