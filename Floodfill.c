#include "Floodfill.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    FloodfillCell elements[FLOODFILL_MAX_WIDTH * FLOODFILL_MAX_HEIGHT];
    int head;
    int tail;
    int count;
} FloodfillQueue;

static int mazeWidth = 0;
static int mazeHeight = 0;
static int moduleInitialized = 0;
static int distances[FLOODFILL_MAX_HEIGHT][FLOODFILL_MAX_WIDTH];
static unsigned char horizontalWalls[FLOODFILL_MAX_HEIGHT + 1][FLOODFILL_MAX_WIDTH];
static unsigned char verticalWalls[FLOODFILL_MAX_HEIGHT][FLOODFILL_MAX_WIDTH + 1];
static FloodfillCell goalCells[FLOODFILL_MAX_GOALS];
static int goalCellCount = 0;

static void logMessage(const char* text) {
    fprintf(stderr, "%s\n", text);
    fflush(stderr);
}

static void queueInit(FloodfillQueue* queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

static int queueIsEmpty(const FloodfillQueue* queue) {
    return queue->count == 0;
}

static void queuePush(FloodfillQueue* queue, FloodfillCell cell) {
    if (queue->count >= FLOODFILL_MAX_WIDTH * FLOODFILL_MAX_HEIGHT) {
        logMessage("Floodfill queue overflow");
        return;
    }
    queue->elements[queue->tail] = cell;
    queue->tail = (queue->tail + 1) % (FLOODFILL_MAX_WIDTH * FLOODFILL_MAX_HEIGHT);
    queue->count += 1;
}

static FloodfillCell queuePop(FloodfillQueue* queue) {
    FloodfillCell cell = {0, 0};
    if (queueIsEmpty(queue)) {
        return cell;
    }
    cell = queue->elements[queue->head];
    queue->head = (queue->head + 1) % (FLOODFILL_MAX_WIDTH * FLOODFILL_MAX_HEIGHT);
    queue->count -= 1;
    return cell;
}

static int isValidCell(FloodfillCell cell) {
    return cell.x >= 0 && cell.x < mazeWidth && cell.y >= 0 && cell.y < mazeHeight;
}

static FloodfillCell neighborCell(FloodfillCell cell, API_Direction direction) {
    FloodfillCell neighbor = cell;
    switch (direction) {
        case API_DIR_NORTH:
            neighbor.y += 1;
            break;
        case API_DIR_EAST:
            neighbor.x += 1;
            break;
        case API_DIR_SOUTH:
            neighbor.y -= 1;
            break;
        case API_DIR_WEST:
            neighbor.x -= 1;
            break;
    }
    return neighbor;
}

static int isBoundaryEdge(FloodfillCell cell, API_Direction direction) {
    switch (direction) {
        case API_DIR_NORTH:
            return cell.y >= mazeHeight - 1;
        case API_DIR_EAST:
            return cell.x >= mazeWidth - 1;
        case API_DIR_SOUTH:
            return cell.y <= 0;
        case API_DIR_WEST:
            return cell.x <= 0;
    }
    return 1;
}

static unsigned char* wallSlot(FloodfillCell cell, API_Direction direction) {
    switch (direction) {
        case API_DIR_NORTH:
            if (cell.y + 1 <= mazeHeight) {
                return &horizontalWalls[cell.y + 1][cell.x];
            }
            break;
        case API_DIR_EAST:
            if (cell.x + 1 <= mazeWidth) {
                return &verticalWalls[cell.y][cell.x + 1];
            }
            break;
        case API_DIR_SOUTH:
            if (cell.y >= 0) {
                return &horizontalWalls[cell.y][cell.x];
            }
            break;
        case API_DIR_WEST:
            if (cell.x >= 0) {
                return &verticalWalls[cell.y][cell.x];
            }
            break;
    }
    return NULL;
}

static int hasWallBetween(FloodfillCell cell, API_Direction direction) {
    unsigned char* slot = wallSlot(cell, direction);
    if (slot == NULL) {
        return 1;
    }
    return *slot != 0;
}

static void updateNeighborWall(FloodfillCell cell, API_Direction direction, unsigned char value) {
    FloodfillCell neighbor = neighborCell(cell, direction);
    if (!isValidCell(neighbor)) {
        return;
    }
    unsigned char* neighborSlot = NULL;
    switch (direction) {
        case API_DIR_NORTH:
            neighborSlot = wallSlot(neighbor, API_DIR_SOUTH);
            break;
        case API_DIR_EAST:
            neighborSlot = wallSlot(neighbor, API_DIR_WEST);
            break;
        case API_DIR_SOUTH:
            neighborSlot = wallSlot(neighbor, API_DIR_NORTH);
            break;
        case API_DIR_WEST:
            neighborSlot = wallSlot(neighbor, API_DIR_EAST);
            break;
    }
    if (neighborSlot != NULL) {
        *neighborSlot = value;
    }
}

static char directionToChar(API_Direction direction) {
    switch (direction) {
        case API_DIR_NORTH:
            return 'n';
        case API_DIR_EAST:
            return 'e';
        case API_DIR_SOUTH:
            return 's';
        case API_DIR_WEST:
            return 'w';
    }
    return '?';
}

static void displayDistance(FloodfillCell cell, int value) {
    if (!isValidCell(cell)) {
        return;
    }
    if (value < 0) {
        API_clearText(cell.x, cell.y);
        return;
    }
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", value);
    API_setText(cell.x, cell.y, buffer);
}

static void clearAllDistances(void) {
    for (int y = 0; y < mazeHeight; ++y) {
        for (int x = 0; x < mazeWidth; ++x) {
            distances[y][x] = -1;
            API_clearText(x, y);
        }
    }
}

static void setBoundaryWalls(void) {
    for (int x = 0; x < mazeWidth; ++x) {
        horizontalWalls[0][x] = 1;
        horizontalWalls[mazeHeight][x] = 1;
    }
    for (int y = 0; y < mazeHeight; ++y) {
        verticalWalls[y][0] = 1;
        verticalWalls[y][mazeWidth] = 1;
    }
}

void Floodfill_init(void) {
    mazeWidth = API_mazeWidth();
    mazeHeight = API_mazeHeight();
    if (mazeWidth > FLOODFILL_MAX_WIDTH) {
        logMessage("Maze width exceeds FLOODFILL_MAX_WIDTH; truncating");
        mazeWidth = FLOODFILL_MAX_WIDTH;
    }
    if (mazeHeight > FLOODFILL_MAX_HEIGHT) {
        logMessage("Maze height exceeds FLOODFILL_MAX_HEIGHT; truncating");
        mazeHeight = FLOODFILL_MAX_HEIGHT;
    }
    memset(horizontalWalls, 0, sizeof(horizontalWalls));
    memset(verticalWalls, 0, sizeof(verticalWalls));
    clearAllDistances();
    setBoundaryWalls();
    moduleInitialized = 1;

    FloodfillCell defaults[FLOODFILL_MAX_GOALS];
    int count = 0;
    int xLow = (mazeWidth - 1) / 2;
    int xHigh = mazeWidth / 2;
    int yLow = (mazeHeight - 1) / 2;
    int yHigh = mazeHeight / 2;

    defaults[count++] = (FloodfillCell){xLow, yLow};
    if (xHigh != xLow) {
        defaults[count++] = (FloodfillCell){xHigh, yLow};
    }
    if (yHigh != yLow) {
        defaults[count++] = (FloodfillCell){xLow, yHigh};
        if (xHigh != xLow) {
            defaults[count++] = (FloodfillCell){xHigh, yHigh};
        }
    }

    Floodfill_setGoals(defaults, count);
    logMessage("Floodfill initialized");
}

void Floodfill_setGoals(const FloodfillCell* goals, int goalCountInput) {
    if (!moduleInitialized) {
        return;
    }
    goalCellCount = 0;
    if (goals == NULL || goalCountInput <= 0) {
        logMessage("Floodfill_setGoals called with no goals");
        return;
    }
    for (int i = 0; i < goalCountInput && goalCellCount < FLOODFILL_MAX_GOALS; ++i) {
        FloodfillCell candidate = goals[i];
        if (!isValidCell(candidate)) {
            continue;
        }
        bool duplicate = false;
        for (int existing = 0; existing < goalCellCount; ++existing) {
            if (goalCells[existing].x == candidate.x && goalCells[existing].y == candidate.y) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            goalCells[goalCellCount++] = candidate;
        }
    }
    if (goalCellCount == 0) {
        logMessage("Floodfill_setGoals found no valid goals");
        return;
    }
    Floodfill_recalculate();
}

void Floodfill_markWall(FloodfillCell cell, API_Direction direction, int present) {
    if (!moduleInitialized || !isValidCell(cell)) {
        return;
    }
    unsigned char value = present ? 1 : 0;
    unsigned char* slot = wallSlot(cell, direction);
    if (slot == NULL) {
        return;
    }
    if (present == 0 && isBoundaryEdge(cell, direction)) {
        return;
    }
    if (*slot == value) {
        return;
    }
    *slot = value;
    updateNeighborWall(cell, direction, value);
    char dirChar = directionToChar(direction);
    if (present) {
        API_setWall(cell.x, cell.y, dirChar);
    } else {
        API_clearWall(cell.x, cell.y, dirChar);
    }
}

void Floodfill_recalculate(void) {
    if (!moduleInitialized || goalCellCount == 0) {
        return;
    }
    clearAllDistances();

    FloodfillQueue queue;
    queueInit(&queue);

    for (int i = 0; i < goalCellCount; ++i) {
        FloodfillCell goal = goalCells[i];
        if (!isValidCell(goal)) {
            continue;
        }
        distances[goal.y][goal.x] = 0;
        displayDistance(goal, 0);
        queuePush(&queue, goal);
    }

    while (!queueIsEmpty(&queue)) {
        FloodfillCell current = queuePop(&queue);
        int currentDistance = distances[current.y][current.x];
        for (API_Direction dir = API_DIR_NORTH; dir <= API_DIR_WEST; dir = (API_Direction)(dir + 1)) {
            if (hasWallBetween(current, dir)) {
                continue;
            }
            FloodfillCell neighbor = neighborCell(current, dir);
            if (!isValidCell(neighbor)) {
                continue;
            }
            if (distances[neighbor.y][neighbor.x] != -1) {
                continue;
            }
            distances[neighbor.y][neighbor.x] = currentDistance + 1;
            displayDistance(neighbor, distances[neighbor.y][neighbor.x]);
            queuePush(&queue, neighbor);
        }
    }
}

int Floodfill_distanceAt(FloodfillCell cell) {
    if (!moduleInitialized || !isValidCell(cell)) {
        return -1;
    }
    return distances[cell.y][cell.x];
}

int Floodfill_canMove(FloodfillCell cell, API_Direction direction) {
    if (!moduleInitialized || !isValidCell(cell)) {
        return 0;
    }
    if (hasWallBetween(cell, direction)) {
        return 0;
    }
    FloodfillCell neighbor = neighborCell(cell, direction);
    if (!isValidCell(neighbor)) {
        return 0;
    }
    return 1;
}

FloodfillCell Floodfill_neighbor(FloodfillCell cell, API_Direction direction) {
    FloodfillCell neighbor = neighborCell(cell, direction);
    if (!moduleInitialized || !isValidCell(neighbor)) {
        return (FloodfillCell){-1, -1};
    }
    return neighbor;
}
