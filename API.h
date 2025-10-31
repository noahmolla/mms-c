#pragma once

typedef enum {
	API_DIR_NORTH = 0,
	API_DIR_EAST,
	API_DIR_SOUTH,
	API_DIR_WEST
} API_Direction;

int API_mazeWidth();
int API_mazeHeight();

int API_wallFront();
int API_wallRight();
int API_wallLeft();

int API_moveForward();  // Returns 0 if crash, else returns 1
void API_turnRight();
void API_turnLeft();

void API_initMouseTracking();
int API_mouseX();
int API_mouseY();
API_Direction API_mouseHeading();

void API_setWall(int x, int y, char direction);
void API_clearWall(int x, int y, char direction);

void API_setColor(int x, int y, char color);
void API_clearColor(int x, int y);
void API_clearAllColor();

void API_setText(int x, int y, char* str);
void API_clearText(int x, int y);
void API_clearAllText();

int API_wasReset();
void API_ackReset();
