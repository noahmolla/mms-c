#include <limits.h>
#include <stdio.h>

#include "API.h"
#include "Floodfill.h"

#define MAX_PATH_LENGTH (FLOODFILL_MAX_WIDTH * FLOODFILL_MAX_HEIGHT)

typedef enum {
    PHASE_TO_CENTER = 0,
    PHASE_TO_START,
    PHASE_DONE
} NavigationPhase;

static NavigationPhase navigationPhase = PHASE_TO_CENTER;
static FloodfillCell centerGoals[FLOODFILL_MAX_GOALS];
static int centerGoalCount = 0;
static FloodfillCell currentGoals[FLOODFILL_MAX_GOALS];
static int currentGoalCount = 0;
static const FloodfillCell START_GOAL = {0, 0};
static API_Direction fastPath[MAX_PATH_LENGTH];
static int fastPathLength = 0;

static void debugLog(const char* text) {
    fprintf(stderr, "%s\n", text);
    fflush(stderr);
}

static API_Direction rotateLeft(API_Direction direction) {
    return (API_Direction)((direction + 3) % 4);
}

static API_Direction rotateRight(API_Direction direction) {
    return (API_Direction)((direction + 1) % 4);
}

static API_Direction rotateBack(API_Direction direction) {
    return (API_Direction)((direction + 2) % 4);
}

static int cellsEqual(FloodfillCell a, FloodfillCell b) {
    return a.x == b.x && a.y == b.y;
}

static int isCellInList(FloodfillCell cell, const FloodfillCell* list, int count) {
    for (int i = 0; i < count; ++i) {
        if (cellsEqual(cell, list[i])) {
            return 1;
        }
    }
    return 0;
}

static int isCenterCell(FloodfillCell cell) {
    return isCellInList(cell, centerGoals, centerGoalCount);
}

static void applyGoals(const FloodfillCell* goals, int count) {
    if (count < 0) {
        count = 0;
    }
    if (count > FLOODFILL_MAX_GOALS) {
        count = FLOODFILL_MAX_GOALS;
    }
    currentGoalCount = count;
    for (int i = 0; i < currentGoalCount; ++i) {
        currentGoals[i] = goals[i];
    }
    Floodfill_setGoals(goals, currentGoalCount);
}

static void computeCenterGoals(void) {
    int width = API_mazeWidth();
    int height = API_mazeHeight();

    centerGoalCount = 0;
    int xLow = (width - 1) / 2;
    int xHigh = width / 2;
    int yLow = (height - 1) / 2;
    int yHigh = height / 2;

    centerGoals[centerGoalCount++] = (FloodfillCell){xLow, yLow};
    if (xHigh != xLow) {
        centerGoals[centerGoalCount++] = (FloodfillCell){xHigh, yLow};
    }
    if (yHigh != yLow) {
        centerGoals[centerGoalCount++] = (FloodfillCell){xLow, yHigh};
        if (xHigh != xLow) {
            centerGoals[centerGoalCount++] = (FloodfillCell){xHigh, yHigh};
        }
    }
}

static void updateNavigationPhase(FloodfillCell current) {
    if (navigationPhase == PHASE_TO_CENTER) {
        if (isCenterCell(current)) {
            debugLog("Reached center; targeting start");
            applyGoals(&START_GOAL, 1);
            navigationPhase = PHASE_TO_START;
        }
        return;
    }

    if (navigationPhase == PHASE_TO_START) {
        if (cellsEqual(current, START_GOAL)) {
            debugLog("Returned to start; run complete");
            navigationPhase = PHASE_DONE;
        }
    }
}

static int rotationCost(API_Direction target, API_Direction heading) {
    int diff = (target - heading + 4) % 4;
    if (diff == 0) {
        return 0;
    }
    if (diff == 1 || diff == 3) {
        return 1;
    }
    return 2;
}

static void rotateTo(API_Direction target) {
    API_Direction heading = API_mouseHeading();
    while (heading != target) {
        int diff = (target - heading + 4) % 4;
        if (diff == 1) {
            API_turnRight();
        } else if (diff == 3) {
            API_turnLeft();
        } else {
            API_turnLeft();
            API_turnLeft();
        }
        heading = API_mouseHeading();
    }
}

static void senseWallsAndFlood(void) {
    FloodfillCell current = {API_mouseX(), API_mouseY()};
    API_Direction heading = API_mouseHeading();

    Floodfill_markWall(current, heading, API_wallFront() ? 1 : 0);
    Floodfill_markWall(current, rotateLeft(heading), API_wallLeft() ? 1 : 0);
    Floodfill_markWall(current, rotateRight(heading), API_wallRight() ? 1 : 0);

    Floodfill_recalculate();
}

static API_Direction chooseNextDirection(FloodfillCell current, API_Direction heading) {
    int currentDistance = Floodfill_distanceAt(current);
    int bestDistance = INT_MAX;
    int bestRotation = INT_MAX;
    API_Direction bestDirection = heading;
    int foundBetter = 0;

    for (API_Direction dir = API_DIR_NORTH; dir <= API_DIR_WEST; dir = (API_Direction)(dir + 1)) {
        if (!Floodfill_canMove(current, dir)) {
            continue;
        }
        FloodfillCell neighbor = Floodfill_neighbor(current, dir);
        int neighborDistance = Floodfill_distanceAt(neighbor);
        if (neighborDistance < 0) {
            continue;
        }
        int rotCost = rotationCost(dir, heading);
        if (currentDistance >= 0 && neighborDistance < currentDistance) {
            if (!foundBetter || neighborDistance < bestDistance ||
                (neighborDistance == bestDistance && rotCost < bestRotation)) {
                foundBetter = 1;
                bestDistance = neighborDistance;
                bestRotation = rotCost;
                bestDirection = dir;
            }
            continue;
        }
        if (foundBetter) {
            continue;
        }
        if (neighborDistance < bestDistance ||
            (neighborDistance == bestDistance && rotCost < bestRotation)) {
            bestDistance = neighborDistance;
            bestRotation = rotCost;
            bestDirection = dir;
        }
    }

    return bestDirection;
}

static int buildFastPath(void) {
    if (centerGoalCount == 0) {
        debugLog("Fast path build failed: no center goals");
        return 0;
    }

    applyGoals(centerGoals, centerGoalCount);
    Floodfill_recalculate();

    FloodfillCell current = START_GOAL;
    API_Direction heading = API_mouseHeading();
    fastPathLength = 0;

    int safety = MAX_PATH_LENGTH;
    while (!isCenterCell(current)) {
        if (safety-- <= 0) {
            debugLog("Fast path build aborted: exceeded length limit");
            return 0;
        }
        int currentDistance = Floodfill_distanceAt(current);
        if (currentDistance <= 0) {
            if (currentDistance == 0 && isCenterCell(current)) {
                break;
            }
            debugLog("Fast path build failed: invalid distance");
            return 0;
        }

        int bestRotation = INT_MAX;
        API_Direction chosenDir = heading;
        int found = 0;
        for (API_Direction dir = API_DIR_NORTH; dir <= API_DIR_WEST; dir = (API_Direction)(dir + 1)) {
            if (!Floodfill_canMove(current, dir)) {
                continue;
            }
            FloodfillCell neighbor = Floodfill_neighbor(current, dir);
            int neighborDistance = Floodfill_distanceAt(neighbor);
            if (neighborDistance < 0 || neighborDistance >= currentDistance) {
                continue;
            }
            int rot = rotationCost(dir, heading);
            if (!found || rot < bestRotation) {
                bestRotation = rot;
                chosenDir = dir;
                found = 1;
            }
        }

        if (!found) {
            debugLog("Fast path build failed: no descending neighbor");
            return 0;
        }

        if (fastPathLength >= MAX_PATH_LENGTH) {
            debugLog("Fast path build failed: path buffer overflow");
            return 0;
        }
        fastPath[fastPathLength++] = chosenDir;
        heading = chosenDir;
        current = Floodfill_neighbor(current, chosenDir);
    }

    return 1;
}

static void executeFastRun(void) {
    debugLog("Starting fast run toward center");
    if (!buildFastPath()) {
        debugLog("Fast run aborted: unable to build path");
        return;
    }

    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Fast path length: %d", fastPathLength);
    debugLog(logBuffer);

    for (int i = 0; i < fastPathLength; ++i) {
        API_Direction stepDir = fastPath[i];
        rotateTo(stepDir);
        if (!API_moveForward()) {
            debugLog("Fast run halted: move failed");
            return;
        }
    }

    debugLog("Fast run complete");
}

int main(int argc, char* argv[]) {
    debugLog("Running...");
    API_setColor(0, 0, 'G');
    API_initMouseTracking();
    Floodfill_init();
    computeCenterGoals();
    applyGoals(centerGoals, centerGoalCount);
    navigationPhase = PHASE_TO_CENTER;
    while (1) {
        senseWallsAndFlood();
        FloodfillCell current = {API_mouseX(), API_mouseY()};
        updateNavigationPhase(current);
        if (navigationPhase == PHASE_DONE) {
            break;
        }
        API_Direction heading = API_mouseHeading();

        API_Direction targetDirection = chooseNextDirection(current, heading);
        rotateTo(targetDirection);

        if (!Floodfill_canMove(current, targetDirection)) {
            Floodfill_markWall(current, targetDirection, 1);
            continue;
        }

        if (!API_moveForward()) {
            Floodfill_markWall(current, targetDirection, 1);
            continue;
        }

        FloodfillCell updated = {API_mouseX(), API_mouseY()};
        Floodfill_markWall(updated, rotateBack(API_mouseHeading()), 0);
    }
    debugLog("Navigation loop exited");
    executeFastRun();
}
