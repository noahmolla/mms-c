#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "API.h"

#define BUFFER_SIZE 32

static int trackingInitialized = 0;
static int mouseX = 0;
static int mouseY = 0;
static API_Direction mouseHeading = API_DIR_NORTH;

static void logMessage(const char* text) {
    fprintf(stderr, "%s\n", text);
    fflush(stderr);
}

static const char* headingToString(API_Direction direction) {
    switch (direction) {
        case API_DIR_NORTH:
            return "north";
        case API_DIR_EAST:
            return "east";
        case API_DIR_SOUTH:
            return "south";
        case API_DIR_WEST:
            return "west";
    }
    return "unknown";
}

static void publishPosition(void) {
    if (!trackingInitialized) {
        return;
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d,%d", mouseX, mouseY);
    API_setText(mouseX, mouseY, buffer);
}

static void updatePosition(void) {
    switch (mouseHeading) {
        case API_DIR_NORTH:
            mouseY += 1;
            break;
        case API_DIR_EAST:
            mouseX += 1;
            break;
        case API_DIR_SOUTH:
            mouseY -= 1;
            break;
        case API_DIR_WEST:
            mouseX -= 1;
            break;
    }
}

int getInteger(char* command) {
    printf("%s\n", command);
    fflush(stdout);
    char response[BUFFER_SIZE];
    fgets(response, BUFFER_SIZE, stdin);
    int value = atoi(response);
    return value;
}

int getBoolean(char* command) {
    printf("%s\n", command);
    fflush(stdout);
    char response[BUFFER_SIZE];
    fgets(response, BUFFER_SIZE, stdin);
    int value = (strcmp(response, "true\n") == 0);
    return value;
}

int getAck(char* command) {
    printf("%s\n", command);
    fflush(stdout);
    char response[BUFFER_SIZE];
    fgets(response, BUFFER_SIZE, stdin);
    int success = (strcmp(response, "ack\n") == 0);
    return success;
}

int API_mazeWidth() {
    return getInteger("mazeWidth");
}

int API_mazeHeight() {
    return getInteger("mazeHeight");
}

int API_wallFront() {
    return getBoolean("wallFront");
}

int API_wallRight() {
    return getBoolean("wallRight");
}

int API_wallLeft() {
    return getBoolean("wallLeft");
}

int API_moveForward() {
    int success = getAck("moveForward");
    if (!success) {
        logMessage("moveForward failed (no ack)");
        return success;
    }
    if (!trackingInitialized) {
        return success;
    }
    updatePosition();
    publishPosition();
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Moved to (%d, %d)", mouseX, mouseY);
    logMessage(logBuffer);
    return success;
}

void API_turnRight() {
    int success = getAck("turnRight");
    if (!success) {
        logMessage("turnRight failed (no ack)");
        return;
    }
    if (!trackingInitialized) {
        return;
    }
    mouseHeading = (API_Direction)((mouseHeading + 1) % 4);
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Turned right; heading %s", headingToString(mouseHeading));
    logMessage(logBuffer);
}

void API_turnLeft() {
    int success = getAck("turnLeft");
    if (!success) {
        logMessage("turnLeft failed (no ack)");
        return;
    }
    if (!trackingInitialized) {
        return;
    }
    mouseHeading = (API_Direction)((mouseHeading + 3) % 4);
    char logBuffer[64];
    snprintf(logBuffer, sizeof(logBuffer), "Turned left; heading %s", headingToString(mouseHeading));
    logMessage(logBuffer);
}

void API_initMouseTracking() {
    mouseX = 0;
    mouseY = 0;
    mouseHeading = API_DIR_NORTH;
    trackingInitialized = 1;
    publishPosition();
    logMessage("Mouse tracking initialized at (0, 0)");
}

int API_mouseX() {
    return mouseX;
}

int API_mouseY() {
    return mouseY;
}

API_Direction API_mouseHeading() {
    return mouseHeading;
}

void API_setWall(int x, int y, char direction) {
    printf("setWall %d %d %c\n", x, y, direction);
    fflush(stdout);
}

void API_clearWall(int x, int y, char direction) {
    printf("clearWall %d %d %c\n", x, y, direction);
    fflush(stdout);
}

void API_setColor(int x, int y, char color) {
    printf("setColor %d %d %c\n", x, y, color);
    fflush(stdout);
}

void API_clearColor(int x, int y) {
    printf("clearColor %d %d\n", x, y);
    fflush(stdout);
}

void API_clearAllColor() {
    printf("clearAllColor\n");
    fflush(stdout);
}

void API_setText(int x, int y, char* text) {
    printf("setText %d %d %s\n", x, y, text);
    fflush(stdout);
}

void API_clearText(int x, int y) {
    printf("clearText %d %d\n", x, y);
    fflush(stdout);
}

void API_clearAllText() {
    printf("clearAllText\n");
    fflush(stdout);
}

int API_wasReset() {
    return getBoolean("wasReset");
}

void API_ackReset() {
    getAck("ackReset");
}
