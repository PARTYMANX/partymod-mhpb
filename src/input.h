#ifndef _INPUT_H_
#define _INPUT_H_

void initInput();
void installInputPatches();
//int processInputEvent(SDL_Event *e);
void installModuleInputPatches(int module, uint32_t baseAddr);

#endif