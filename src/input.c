#include <stdio.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <global.h>
#include <patch.h>
#include <event.h>
#include <gfx/gfx.h>
#include <config.h>
#include <log.h>

//

struct keybinds {
	SDL_Scancode menu;
	SDL_Scancode cameraToggle;

	SDL_Scancode grind;
	SDL_Scancode grab;
	SDL_Scancode ollie;
	SDL_Scancode kick;

	SDL_Scancode leftSpin;
	SDL_Scancode rightSpin;
	SDL_Scancode nollie;
	SDL_Scancode switchRevert;

	SDL_Scancode right;
	SDL_Scancode left;
	SDL_Scancode up;
	SDL_Scancode down;
};

// a recreation of the SDL_GameControllerButton enum, but with the addition of right/left trigger
typedef enum {
	CONTROLLER_UNBOUND = -1,
	CONTROLLER_BUTTON_A = SDL_CONTROLLER_BUTTON_A,
	CONTROLLER_BUTTON_B = SDL_CONTROLLER_BUTTON_B,
	CONTROLLER_BUTTON_X = SDL_CONTROLLER_BUTTON_X,
	CONTROLLER_BUTTON_Y = SDL_CONTROLLER_BUTTON_Y,
	CONTROLLER_BUTTON_BACK = SDL_CONTROLLER_BUTTON_BACK,
	CONTROLLER_BUTTON_GUIDE = SDL_CONTROLLER_BUTTON_GUIDE,
	CONTROLLER_BUTTON_START = SDL_CONTROLLER_BUTTON_START,
	CONTROLLER_BUTTON_LEFTSTICK = SDL_CONTROLLER_BUTTON_LEFTSTICK,
	CONTROLLER_BUTTON_RIGHTSTICK = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
	CONTROLLER_BUTTON_LEFTSHOULDER = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
	CONTROLLER_BUTTON_RIGHTSHOULDER = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
	CONTROLLER_BUTTON_DPAD_UP = SDL_CONTROLLER_BUTTON_DPAD_UP,
	CONTROLLER_BUTTON_DPAD_DOWN = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
	CONTROLLER_BUTTON_DPAD_LEFT = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
	CONTROLLER_BUTTON_DPAD_RIGHT = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
	CONTROLLER_BUTTON_MISC1 = SDL_CONTROLLER_BUTTON_MISC1,
	CONTROLLER_BUTTON_PADDLE1 = SDL_CONTROLLER_BUTTON_PADDLE1,
	CONTROLLER_BUTTON_PADDLE2 = SDL_CONTROLLER_BUTTON_PADDLE2,
	CONTROLLER_BUTTON_PADDLE3 = SDL_CONTROLLER_BUTTON_PADDLE3,
	CONTROLLER_BUTTON_PADDLE4 = SDL_CONTROLLER_BUTTON_PADDLE4,
	CONTROLLER_BUTTON_TOUCHPAD = SDL_CONTROLLER_BUTTON_TOUCHPAD,
	CONTROLLER_BUTTON_RIGHTTRIGGER = 21,
	CONTROLLER_BUTTON_LEFTTRIGGER = 22,
} controllerButton;

typedef enum {
	CONTROLLER_STICK_UNBOUND = -1,
	CONTROLLER_STICK_LEFT = 0,
	CONTROLLER_STICK_RIGHT = 1
} controllerStick;

struct controllerbinds {
	controllerButton menu;
	controllerButton cameraToggle;

	controllerButton grind;
	controllerButton grab;
	controllerButton ollie;
	controllerButton kick;

	controllerButton leftSpin;
	controllerButton rightSpin;
	controllerButton nollie;
	controllerButton switchRevert;

	controllerButton right;
	controllerButton left;
	controllerButton up;
	controllerButton down;

	controllerStick movement;
};

//

int controllerCount;
int controllerListSize;
SDL_GameController **controllerList;
int activeController = -1;
struct keybinds keybinds;
struct controllerbinds padbinds;

uint8_t isCursorActive = 1;
uint8_t isUsingKeyboard = 1;
uint8_t isUsingHardCodeControls = 1;
uint8_t anyButtonPressed = 0;
uint8_t isVibrationActive = 1;

void setUsingKeyboard(uint8_t usingKeyboard) {
	isUsingKeyboard = usingKeyboard;

	if (usingKeyboard) {
		activeController = -1;
	}

	if (isUsingKeyboard && isCursorActive) {
		SDL_ShowCursor(SDL_ENABLE);
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void setCursorActive() {
	isCursorActive = 1;

	if (isUsingKeyboard && isCursorActive) {
		SDL_ShowCursor(SDL_ENABLE);
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void setCursorInactive() {
	isCursorActive = 0;

	if (isUsingKeyboard && isCursorActive) {
		SDL_ShowCursor(SDL_ENABLE);
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void addController(int idx) {
	log_printf(LL_INFO, "Detected controller \"%s\"\n", SDL_GameControllerNameForIndex(idx));

	SDL_GameController *controller = SDL_GameControllerOpen(idx);

	if (controller) {
		if (controllerCount == controllerListSize) {
			int tmpSize = controllerListSize + 1;
			SDL_GameController **tmp = realloc(controllerList, sizeof(SDL_GameController *) * tmpSize);
			if (!tmp) {
				return; // TODO: log something here or otherwise do something
			}

			controllerListSize = tmpSize;
			controllerList = tmp;
		}

		controllerList[controllerCount] = controller;
		//activeController = controllerCount;
		controllerCount++;
	}
}

void removeController(SDL_GameController *controller) {
	log_printf(LL_INFO, "Controller \"%s\" disconnected\n", SDL_GameControllerName(controller));

	int i = 0;

	while (i < controllerCount && controllerList[i] != controller) {
		i++;
	}

	if (controllerList[i] == controller) {
		SDL_GameControllerClose(controller);

		for (; i < controllerCount - 1; i++) {
			controllerList[i] = controllerList[i + 1];
		}
		controllerCount--;

		if (activeController == i) {
			activeController = -1;
		}
	} else {
		log_printf(LL_WARN, "Did not find disconnected controller in list\n");
	}
}

void setActiveController(SDL_GameController *controller) {
	int i = 0;

	while (i < controllerCount && controllerList[i] != controller) {
		i++;
	}

	if (controllerList[i] == controller) {
		activeController = i;
	}
}

void initSDLControllers() {
	log_printf(LL_INFO, "Initializing Controller Input\n");

	controllerCount = 0;
	controllerListSize = 1;
	controllerList = malloc(sizeof(SDL_GameController *) * controllerListSize);

	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			addController(i);
		}
	}

	// add event filter for newly connected controllers
	//SDL_SetEventFilter(controllerEventFilter, NULL);
}

uint8_t axisAbs(uint8_t val) {
	if (val > 127) {
		// positive, just remove top bit
		return val & 0x7F;
	} else {
		// negative
		return ~val & 0x7f;
	}
}

uint8_t getButton(SDL_GameController *controller, controllerButton button) {
	if (button == CONTROLLER_BUTTON_LEFTTRIGGER) {
		uint8_t pressure = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7;
		return pressure > 0x80;
	} else if (button == CONTROLLER_BUTTON_RIGHTTRIGGER) {
		uint8_t pressure = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7;
		return pressure > 0x80;
	} else {
		return SDL_GameControllerGetButton(controller, button);
	}
}

void getStick(SDL_GameController *controller, controllerStick stick, int16_t *xOut, int16_t *yOut) {
	if (stick == CONTROLLER_STICK_LEFT) {
		*xOut = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
		*yOut = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
	} else if (stick == CONTROLLER_STICK_RIGHT) {
		*xOut = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
		*yOut = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);
	} else {
		*xOut = 0;
		*yOut = 0;
	}
}

uint16_t pollController(SDL_GameController *controller) {
	uint16_t result = 0;

	if (SDL_GameControllerGetAttached(controller)) {
		//printf("Polling controller \"%s\"\n", SDL_GameControllerName(controller));

		// buttons
		if (getButton(controller, padbinds.menu)) {
			result |= 0x01 << 11;
		}
		if (getButton(controller, padbinds.cameraToggle)) {
			result |= 0x01 << 8;
		}

		if (getButton(controller, padbinds.grind)) {
			result |= 0x01 << 4;
		}
		if (getButton(controller, padbinds.grab)) {
			result |= 0x01 << 5;
		}
		if (getButton(controller, padbinds.ollie)) {
			result |= 0x01 << 6;
		}
		if (getButton(controller, padbinds.kick)) {
			result |= 0x01 << 7;
		}

		// shoulders
		if (getButton(controller, padbinds.leftSpin)) {
			result |= 0x01 << 2;
		}

		if (getButton(controller, padbinds.rightSpin)) {
			result |= 0x01 << 3;
		}

		if (getButton(controller, padbinds.nollie)) {
			result |= 0x01 << 0;
		}

		if (getButton(controller, padbinds.switchRevert)) {
			result |= 0x01 << 1;
		}
		
		// d-pad
		if (SDL_GameControllerGetButton(controller, padbinds.up)) {
			result |= 0x01 << 12;
		}
		if (SDL_GameControllerGetButton(controller, padbinds.right)) {
			result |= 0x01 << 13;
		}
		if (SDL_GameControllerGetButton(controller, padbinds.down)) {
			result |= 0x01 << 14;
		}
		if (SDL_GameControllerGetButton(controller, padbinds.left)) {
			result |= 0x01 << 15;
		}

		int16_t moveX, moveY;
		getStick(controller, padbinds.movement, &moveX, &moveY);
		if (moveX > 16383) {
			result |= 0x01 << 13;	// right
		} else if (moveX < -16383) {
			result |= 0x01 << 15;	// left
		}

		if (moveY > 16383) {
			result |= 0x01 << 14;	// up
		} else if (moveY < -16383) {
			result |= 0x01 << 12;	// down
		}

		if (result) {
			anyButtonPressed |= 1;
		}
	}

	return result;
}

uint32_t escState = 0;

uint16_t pollKeyboard() {
	//int *gShellMode = 0x006a35b4;
	//uint8_t *isMenuOpen = 0x0069e050;

	uint32_t numKeys = 0;
	uint8_t *keyboardState = SDL_GetKeyboardState(&numKeys);

	// check keyboard state 
	for (int i = 0; i < numKeys; i++) {
		if (keyboardState[i]) {
			anyButtonPressed |= 1;
		}
	}

	//printf("gShellMode: %d\n", *isMenuOpen);

	uint16_t result = 0;

	// slight deviation from original behavior: esc goes back in pause/end run menu
	// to prevent esc double-pressing on menu transitions, process its state here.  
	// escState = 1 means esc was pressed in menu, 2 means pressed out of menu, 0 is unpressed
	if (keyboardState[SDL_SCANCODE_ESCAPE] && !escState) {
		//escState = (*isMenuOpen) ? 1 : 2;
	} else if (!keyboardState[SDL_SCANCODE_ESCAPE]) {
		//escState = 0;
	}

	// buttons
	if (keyboardState[keybinds.menu] || escState == 2) {	// is esc is bound to menu, it will only interfere with hardcoded keybinds.  similar effect on enter but i can't detect the things needed to work there
		result |= 0x01 << 11;
	}
	if (keyboardState[keybinds.cameraToggle]) {
		result |= 0x01 << 8;
	}

	if (keyboardState[keybinds.grind] || escState == 1) {
		result |= 0x01 << 4;
	}
	if (keyboardState[keybinds.grab]) {
		result |= 0x01 << 5;
	}
	if (keyboardState[keybinds.ollie] || keyboardState[SDL_SCANCODE_RETURN]) {
		result |= 0x01 << 6;
	}
	if (keyboardState[keybinds.kick]) {
		result |= 0x01 << 7;
	}

	// shoulders
	if (keyboardState[keybinds.leftSpin]) {
		result |= 0x01 << 2;
	}
	if (keyboardState[keybinds.rightSpin]) {
		result |= 0x01 << 3;
	}
	if (keyboardState[keybinds.nollie]) {
		result |= 0x01 << 0;
	}
	if (keyboardState[keybinds.switchRevert]) {
		result |= 0x01 << 1;
	}
		
	// d-pad
	if (keyboardState[keybinds.up] || keyboardState[SDL_SCANCODE_UP]) {
		result |= 0x01 << 12;
	}
	if (keyboardState[keybinds.right] || keyboardState[SDL_SCANCODE_RIGHT]) {
		result |= 0x01 << 13;
	}
	if (keyboardState[keybinds.down] || keyboardState[SDL_SCANCODE_DOWN]) {
		result |= 0x01 << 14;
	}
	if (keyboardState[keybinds.left] || keyboardState[SDL_SCANCODE_LEFT]) {
		result |= 0x01 << 15;
	}

	return result;
}

// returns 1 if a text entry prompt is on-screen so that keybinds don't interfere with text entry confirmation/cancellation
int isKeyboardTyping() {
	return 0;
}

uint16_t curControlData = 0x0000;
uint16_t oldControlData = 0x0000;
uint16_t buttonsDown = 0x0000;

void __cdecl processController() {
	//int *gShellMode = 0x006a35b4;
	oldControlData = curControlData;

	anyButtonPressed &= 2;
	curControlData = 0;

	if (!isKeyboardTyping()) {
		curControlData |= pollKeyboard();
	}

	// TODO: maybe smart selection of active controller?
	for (int i = 0; i < controllerCount; i++) {
		curControlData |= pollController(controllerList[i]);
	}

	buttonsDown = curControlData & (curControlData ^ oldControlData);
}

uint32_t click_vblank = 0;

uint8_t curMouse = 0;
uint8_t oldMouse = 0;
uint8_t mouseDown = 0;
uint8_t mouseHasMoved = 0;

int processMouse() {
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	mouseHasMoved = 0;

	//uint32_t *vblanks = 0x0056af7c;
	if (currentModule != 1) {
		setCursorActive();
	} else if (currentModule == 1) {
		setCursorInactive();
	}

	uint8_t oldMouse = curMouse;

	uint32_t mouseX, mouseY, resX, resY;
	curMouse = SDL_GetMouseState(&mouseX, &mouseY);

	mouseDown = curMouse & (curMouse ^ oldMouse);

	if (currentModule != 1) {
		toGameScreenCoord(mouseX, mouseY, &mouseX, &mouseY);

		mouseX *= 640;
		mouseY *= 480;
	
		getGameResolution(&resX, &resY);
		mouseX /= resX;
		mouseY /= resY;

	
		uint32_t *gameMouseX[] = { baseAddr + 0x0018d01c, baseAddr + 0x0, baseAddr + 0x001e6b40 };
		uint32_t *gameMouseY[] = { baseAddr + 0x0018d020, baseAddr + 0x0, baseAddr + 0x001e6b44 };
		uint32_t *alsoGameMouseX[] = { baseAddr + 0x0018c688, baseAddr + 0x0, baseAddr + 0x001e6990 };
		uint32_t *alsoGameMouseY[] = { baseAddr + 0x0018c68c, baseAddr + 0x0, baseAddr + 0x001e6994 };

		if (*(gameMouseX[currentModule]) != mouseX || *(gameMouseY[currentModule]) != mouseY) {
			mouseHasMoved = 1;
		}

		*(gameMouseX[currentModule]) = mouseX;
		*(gameMouseY[currentModule]) = mouseY;
		*(alsoGameMouseX[currentModule]) = mouseX;
		*(alsoGameMouseY[currentModule]) = mouseY;
	}
}

int hasMouseMoved() {
	return mouseHasMoved;
}

void processInputEvent(SDL_Event *e) {
	switch (e->type) {
		case SDL_CONTROLLERDEVICEADDED:
			if (SDL_IsGameController(e->cdevice.which)) {
				addController(e->cdevice.which);
			} else {
				log_printf(LL_INFO, "Not a game controller: %s\n", SDL_JoystickNameForIndex(e->cdevice.which));
			}
			return;
		case SDL_CONTROLLERDEVICEREMOVED: {
			SDL_GameController *controller = SDL_GameControllerFromInstanceID(e->cdevice.which);
			if (controller) {
				removeController(controller);
			}
			return;
		}
		case SDL_JOYDEVICEADDED:
			log_printf(LL_DEBUG, "Joystick added: %s\n", SDL_JoystickNameForIndex(e->jdevice.which));
			setUsingKeyboard(0);
			return;
		case SDL_CONTROLLERBUTTONDOWN:
			setActiveController(SDL_GameControllerFromInstanceID(e->cbutton.which));
			setUsingKeyboard(0);
			return;
		case SDL_CONTROLLERAXISMOTION:
			setActiveController(SDL_GameControllerFromInstanceID(e->caxis.which));
			setUsingKeyboard(0);
			return;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION: {
			setUsingKeyboard(1);
			return;
		}
		case SDL_KEYDOWN: 
			setUsingKeyboard(1);
			return;
		default:
			return 0;
	}
}

#define KEYBIND_SECTION "Keybinds"
#define GAMEPAD_SECTION "Gamepad"

void configureControls() {
	isVibrationActive = getConfigBool(GAMEPAD_SECTION, "EnableVibration", 1);

	// Keyboard
	keybinds.menu = getConfigInt(KEYBIND_SECTION, "Pause", SDL_SCANCODE_P);
	keybinds.cameraToggle = getConfigInt(KEYBIND_SECTION, "ViewToggle", SDL_SCANCODE_O);
	
	keybinds.ollie = getConfigInt(KEYBIND_SECTION, "Ollie", SDL_SCANCODE_KP_2);
	keybinds.kick = getConfigInt(KEYBIND_SECTION, "Flip", SDL_SCANCODE_KP_4);
	keybinds.grab = getConfigInt(KEYBIND_SECTION, "Grab", SDL_SCANCODE_KP_6);
	keybinds.grind = getConfigInt(KEYBIND_SECTION, "Grind", SDL_SCANCODE_KP_8);
	
	keybinds.leftSpin = getConfigInt(KEYBIND_SECTION, "SpinLeft", SDL_SCANCODE_KP_7);
	keybinds.rightSpin = getConfigInt(KEYBIND_SECTION, "SpinRight", SDL_SCANCODE_KP_9);
	keybinds.nollie = getConfigInt(KEYBIND_SECTION, "Nollie", SDL_SCANCODE_KP_1);
	keybinds.switchRevert = getConfigInt(KEYBIND_SECTION, "Switch", SDL_SCANCODE_KP_3);
	
	keybinds.left = getConfigInt(KEYBIND_SECTION, "Left", SDL_SCANCODE_A);
	keybinds.right = getConfigInt(KEYBIND_SECTION, "Right", SDL_SCANCODE_D);
	keybinds.up = getConfigInt(KEYBIND_SECTION, "Up", SDL_SCANCODE_W);
	keybinds.down = getConfigInt(KEYBIND_SECTION, "Down", SDL_SCANCODE_S);

	// Gamepad
	padbinds.menu = getConfigInt(GAMEPAD_SECTION, "Pause", CONTROLLER_BUTTON_START);
	padbinds.cameraToggle = getConfigInt(GAMEPAD_SECTION, "ViewToggle", CONTROLLER_BUTTON_BACK);

	padbinds.ollie = getConfigInt(GAMEPAD_SECTION, "Ollie", CONTROLLER_BUTTON_A);
	padbinds.kick = getConfigInt(GAMEPAD_SECTION, "Flip", CONTROLLER_BUTTON_X);
	padbinds.grab = getConfigInt(GAMEPAD_SECTION, "Grab", CONTROLLER_BUTTON_B);
	padbinds.grind = getConfigInt(GAMEPAD_SECTION, "Grind", CONTROLLER_BUTTON_Y);

	padbinds.leftSpin = getConfigInt(GAMEPAD_SECTION, "SpinLeft", CONTROLLER_BUTTON_LEFTSHOULDER);
	padbinds.rightSpin = getConfigInt(GAMEPAD_SECTION, "SpinRight", CONTROLLER_BUTTON_RIGHTSHOULDER);
	padbinds.nollie = getConfigInt(GAMEPAD_SECTION, "Nollie", CONTROLLER_BUTTON_LEFTTRIGGER);
	padbinds.switchRevert = getConfigInt(GAMEPAD_SECTION, "Switch", CONTROLLER_BUTTON_RIGHTTRIGGER);

	padbinds.left = getConfigInt(GAMEPAD_SECTION, "Left", CONTROLLER_BUTTON_DPAD_LEFT);
	padbinds.right = getConfigInt(GAMEPAD_SECTION, "Right", CONTROLLER_BUTTON_DPAD_RIGHT);
	padbinds.up = getConfigInt(GAMEPAD_SECTION, "Up", CONTROLLER_BUTTON_DPAD_UP);
	padbinds.down = getConfigInt(GAMEPAD_SECTION, "Down", CONTROLLER_BUTTON_DPAD_DOWN);

	padbinds.movement = getConfigInt(GAMEPAD_SECTION, "MovementStick", CONTROLLER_STICK_LEFT);
}

void initInput() {
	log_printf(LL_INFO, "Initializing Input!\n");

	// init sdl here
	SDL_Init(SDL_INIT_GAMECONTROLLER);

	SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "false");

	char *controllerDbPath[filePathBufLen];
	int result = sprintf_s(controllerDbPath, filePathBufLen,"%s%s", executableDirectory, "gamecontrollerdb.txt");

	if (result) {
		result = SDL_GameControllerAddMappingsFromFile(controllerDbPath);
		if (result) {
			log_printf(LL_INFO, "Loaded controller database\n");
		} else {
			log_printf(LL_WARN, "Failed to load %s\n", controllerDbPath);
		}
		
	}

	initSDLControllers();
	configureControls();

	registerEventHandler(processInputEvent);
}

void ReadDirectInput() {
	handleEvents();
	processController();
	processMouse();
}

void PCINPUT_ActuatorOn(uint32_t controllerIdx, uint32_t duration, uint32_t motor, uint32_t str) {
	// turns out most of the parameters don't matter.  the ps1 release only seems to respond to grinds, with full strength high frequency vibration
	// pc release only responds with half-strength on both motors.  i think that's the preferred behavior here
	//str = (uint16_t)(((float)str / 255.0f) * 65535.0f);
	if (isVibrationActive && activeController >= 0) {
		duration *= 66;

		if (motor == 0) {
			SDL_GameControllerRumble(controllerList[activeController], 32767, 32767, duration);
		} else {
			SDL_GameControllerRumble(controllerList[activeController], 32767, 32767, duration);
		}
	}
}

void PCINPUT_ShutDown() {
	//printf("STUB: PCINPUT_ShutDown()\n");
}

void PCINPUT_Load() {
	//printf("STUB: PCINPUT_Load()\n");
}

void PCINPUT_Save() {
	//printf("STUB: PCINPUT_Save()\n");
}

int __cdecl getSomethingIdk(int a) {
	// checks that all buttons are released maybe?  either way i don't think we need it anymore
	//printf("STUB: idk %d\n", a);

	return 0;
}

int __cdecl GetButtonState(int mask, int just, int unk) {
	//printf("GET BUTTON STATE: 0x%08x, %d %d\n", mask, just, unk);
	uint32_t baseAddr = *((uint32_t *)0x539db4);

	uint32_t *forcedButtons[] = { baseAddr + 0x0018c758, baseAddr + 0x0021e8b0, baseAddr + 0x001e6a60 };

	if (*(forcedButtons[currentModule]) & mask) {
		*(forcedButtons[currentModule]) = *(forcedButtons[currentModule]) & ~mask;
		return 1;
	}

	uint16_t controlsMask = 0x0000;

	if (mask & 0x00000001) {
		controlsMask |= 0x01 << 6;	// keyboard enter - translate to x
	}

	if (mask & 0x00000002) {
		controlsMask |= 0x01 << 4;	// keyboard esc - translate to triangle
	}

	if (mask & 0x00000004) {
		controlsMask |= 0x01 << 11;	// keyboard esc - translate to start
	}

	if (mask & 0x00000400) {
		if (mouseDown & SDL_BUTTON_LMASK) {
			return 1;
		}
	}

	if (mask & 0x00000800) {
		if (mouseDown & SDL_BUTTON_RMASK) {
			return 1;
		}
	}

	if (mask & 0x00001000) {
		controlsMask |= 0x01 << 12;	// joystick up
	}

	if (mask & 0x00002000) {
		controlsMask |= 0x01 << 14;	// joystick down
	}

	if (mask & 0x00004000) {
		controlsMask |= 0x01 << 15;	// joystick left
	}

	if (mask & 0x00008000) {
		controlsMask |= 0x01 << 13;	// joystick right
	}

	if (mask & 0x00010000) {
		controlsMask |= 0x01 << 6;	// controller x
	}

	if (mask & 0x00020000) {
		controlsMask |= 0x01 << 4;	// controller triangle
	}

	if (mask & 0x00100000) {
		controlsMask |= 0x01 << 7;	// controller circle
	}

	if (mask & 0x00200000) {
		controlsMask |= 0x01 << 5;	// controller square
	}

	if (mask & 0x00400000) {
		controlsMask |= 0x01 << 6;	// controller x
	}

	if (mask & 0x00800000) {
		controlsMask |= 0x01 << 4;	// controller triangle
	}

	if (mask & 0x01000000) {
		controlsMask |= 0x01 << 2;	// controller l1
	}

	if (mask & 0x02000000) {
		controlsMask |= 0x01 << 0;	// controller l2
	}

	if (mask & 0x04000000) {
		controlsMask |= 0x01 << 3;	// controller r1
	}

	if (mask & 0x08000000) {
		controlsMask |= 0x01 << 1;	// controller r2
	}

	if (mask & 0x10000000) {
		controlsMask |= 0x01 << 8;	// controller select
	}

	if (mask & 0x20000000) {
		controlsMask |= 0x01 << 2;	// controller l1 (maybe keyboard shift?)
	}

	if (just) {
		return (controlsMask & buttonsDown) != 0;
	} else {
		return (controlsMask & curControlData) != 0;
	}
}

void __cdecl GetGameButtonState(uint32_t *current, uint32_t *fresh) {
	ReadDirectInput();

	*current = 0;
	*fresh = 0;

	// directions
	*current |= ((curControlData & 0x01 << 12) != 0) << 0;
	*fresh |= ((buttonsDown & 0x01 << 12) != 0) << 0;

	*current |= ((curControlData & 0x01 << 14) != 0) << 1;
	*fresh |= ((buttonsDown & 0x01 << 14) != 0) << 1;

	*current |= ((curControlData & 0x01 << 15) != 0) << 2;
	*fresh |= ((buttonsDown & 0x01 << 15) != 0) << 2;

	*current |= ((curControlData & 0x01 << 13) != 0) << 3;
	*fresh |= ((buttonsDown & 0x01 << 13) != 0) << 3;

	// face buttons
	*current |= ((curControlData & 0x01 << 4) != 0) << 4;
	*fresh |= ((buttonsDown & 0x01 << 4) != 0) << 4;

	*current |= ((curControlData & 0x01 << 7) != 0) << 5;
	*fresh |= ((buttonsDown & 0x01 << 7) != 0) << 5;

	*current |= ((curControlData & 0x01 << 5) != 0) << 6;
	*fresh |= ((buttonsDown & 0x01 << 5) != 0) << 6;

	*current |= ((curControlData & 0x01 << 6) != 0) << 7;
	*fresh |= ((buttonsDown & 0x01 << 6) != 0) << 7;

	// shoulders
	*current |= ((curControlData & 0x01 << 2) != 0) << 8;
	*fresh |= ((buttonsDown & 0x01 << 2) != 0) << 8;

	*current |= ((curControlData & 0x01 << 0) != 0) << 9;
	*fresh |= ((buttonsDown & 0x01 << 0) != 0) << 9;

	*current |= ((curControlData & 0x01 << 3) != 0) << 10;
	*fresh |= ((buttonsDown & 0x01 << 3) != 0) << 10;

	*current |= ((curControlData & 0x01 << 1) != 0) << 11;
	*fresh |= ((buttonsDown & 0x01 << 1) != 0) << 11;

	// start/select
	*current |= ((curControlData & 0x01 << 11) != 0) << 12;
	*fresh |= ((buttonsDown & 0x01 << 11) != 0) << 12;

	*current |= ((curControlData & 0x01 << 8) != 0) << 13;
	*fresh |= ((buttonsDown & 0x01 << 8) != 0) << 13;
}

int getSkipInput() {
	if (anyButtonPressed == 1) {
		anyButtonPressed |= 2;
		return -1;
	} else {
		if (!(anyButtonPressed & 1)) {
			anyButtonPressed = 0;
		}
		return 0;
	}
}

int fakepollcontroller() {
	return 1;
}

// cheat input patches
int getkeybind(int bind) {
	return bind;
}

int getkeybindstate(int bind, int just) {
	uint32_t current = 0;
	uint32_t fresh = 0;

	//GetGameButtonState(&current, &fresh);
	// directions
	current |= ((curControlData & 0x01 << 12) != 0) << 0;
	fresh |= ((buttonsDown & 0x01 << 12) != 0) << 0;

	current |= ((curControlData & 0x01 << 14) != 0) << 1;
	fresh |= ((buttonsDown & 0x01 << 14) != 0) << 1;

	current |= ((curControlData & 0x01 << 15) != 0) << 2;
	fresh |= ((buttonsDown & 0x01 << 15) != 0) << 2;

	current |= ((curControlData & 0x01 << 13) != 0) << 3;
	fresh |= ((buttonsDown & 0x01 << 13) != 0) << 3;

	// face buttons
	current |= ((curControlData & 0x01 << 4) != 0) << 4;
	fresh |= ((buttonsDown & 0x01 << 4) != 0) << 4;

	current |= ((curControlData & 0x01 << 7) != 0) << 5;
	fresh |= ((buttonsDown & 0x01 << 7) != 0) << 5;

	current |= ((curControlData & 0x01 << 5) != 0) << 6;
	fresh |= ((buttonsDown & 0x01 << 5) != 0) << 6;

	current |= ((curControlData & 0x01 << 6) != 0) << 7;
	fresh |= ((buttonsDown & 0x01 << 6) != 0) << 7;

	// shoulders
	current |= ((curControlData & 0x01 << 2) != 0) << 8;
	fresh |= ((buttonsDown & 0x01 << 2) != 0) << 8;

	current |= ((curControlData & 0x01 << 0) != 0) << 9;
	fresh |= ((buttonsDown & 0x01 << 0) != 0) << 9;

	current |= ((curControlData & 0x01 << 3) != 0) << 10;
	fresh |= ((buttonsDown & 0x01 << 3) != 0) << 10;

	current |= ((curControlData & 0x01 << 1) != 0) << 11;
	fresh |= ((buttonsDown & 0x01 << 1) != 0) << 11;

	// start/select
	current |= ((curControlData & 0x01 << 11) != 0) << 12;
	fresh |= ((buttonsDown & 0x01 << 11) != 0) << 12;

	current |= ((curControlData & 0x01 << 8) != 0) << 13;
	fresh |= ((buttonsDown & 0x01 << 8) != 0) << 13;

	if (just) {
		return (bind & fresh) != 0;
	} else {
		return (bind & current) != 0;
	}
}

void installModuleInputPatches(int module, uint32_t baseAddr) {
	switch(module) {
	case 0:	
		patchJmp(baseAddr + 0x0001a0b0, PCINPUT_Load);	// init directinput
		patchJmp(baseAddr + 0x000205f0, PCINPUT_Load);	// create devices
		patchJmp(baseAddr + 0x00020080, PCINPUT_Load);	// write config
		patchJmp(baseAddr + 0x00020140, GetButtonState);	// get button
		patchJmp(baseAddr + 0x00020690, ReadDirectInput);	// poll keyboard
		patchJmp(baseAddr + 0x0001b050, fakepollcontroller);	// poll controller (skip)
		patchJmp(baseAddr + 0x00023030, hasMouseMoved);
		patchNop(baseAddr + 0x00023011, 5);	// don't draw cursor

		break;
	case 1:
		patchJmp(baseAddr + 0x00070ac0, PCINPUT_Load);	// init directinput
		patchJmp(baseAddr + 0x00076750, PCINPUT_Load);	// create devices
		patchJmp(baseAddr + 0x00075ea0, PCINPUT_Load);	// write config
		patchJmp(baseAddr + 0x00075f60, GetButtonState);	// get button
		patchJmp(baseAddr + 0x000767f0, ReadDirectInput);	// poll keyboard
		patchJmp(baseAddr + 0x000723a0, fakepollcontroller);	// poll controller (skip)
		patchJmp(baseAddr + 0x000763c0, GetGameButtonState);
		patchJmp(baseAddr + 0x000764f0, getkeybind);
		patchJmp(baseAddr + 0x00076800, getkeybindstate);
		break;
	case 2:
		patchJmp(baseAddr + 0x00032b80, PCINPUT_Load);	// init directinput
		patchJmp(baseAddr + 0x000359a0, PCINPUT_Load);	// create devices
		patchJmp(baseAddr + 0x00035430, PCINPUT_Load);	// write config
		patchJmp(baseAddr + 0x000354f0, GetButtonState);	// get button
		patchJmp(baseAddr + 0x00035a40, ReadDirectInput);	// poll keyboard
		patchJmp(baseAddr + 0x00033a40, fakepollcontroller);	// poll controller (skip)
		patchJmp(baseAddr + 0x00036b80, hasMouseMoved);
		//patchByte(baseAddr + 0x00036b43, 0xc3);	// don't draw cursor
		patchByte(baseAddr + 0x00036b17, 0xeb);	// don't draw cursor
		//patchJmp(baseAddr + 0x000763c0, GetGameButtonState);
		break;
	}
}