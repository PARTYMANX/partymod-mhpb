#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define VERSION_NUMBER_MAJOR 1
#define VERSION_NUMBER_MINOR 0
#define VERSION_NUMBER_PATCH 3

#define filePathBufLen 1024

int currentModule;

char executableDirectory[filePathBufLen];
void quitGame();
void fatalError(const char *msg);

#endif