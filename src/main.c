#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <patch.h>
#include <global.h>
#include <input.h>
#include <config.h>
#include <gfx/gfx.h>
#include <mem.h>
#include <event.h>
#include <window.h>
#include <log.h>
#include <util/hash.h>

// disable the options menu entries for control and display options as they're no longer relevant
void __fastcall OptionsMenuConstructorWrapper(uint8_t **optionsMenu) {
	//void (__fastcall *OptionsMenuConstructor)(uint8_t **) = 0x0048185d;
	void (__fastcall *OptionsMenuConstructor)(uint8_t **) = 0x0047eb00;

	OptionsMenuConstructor(optionsMenu);

	//optionsMenu[0xd9][0xc] = 0;
	//optionsMenu[0xda][0xc] = 0;
}

/*void patchOptionsMenu() {

	patchCall(0x0048185d, OptionsMenuConstructorWrapper);
	// get rid of player controls menu
	patchNop(0x0047f1f0, 5);
	patchByte(0x0047f203, 0xeb);

	// get rid of display controls menu
	patchNop(0x0047f22d, 5);
	patchByte(0x0047f240, 0xeb);
}*/

void initPatch() {
	GetModuleFileName(NULL, &executableDirectory, filePathBufLen);

	// find last slash
	char *exe = strrchr(executableDirectory, '\\');
	if (exe) {
		*(exe + 1) = '\0';
	}

	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	initConfig();
	
	int isDebug = getConfigBool("Miscellaneous", "Debug", 0);

	configureLogging(isDebug);

	if (isDebug) {
		AllocConsole();

		FILE *fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
	}
	log_printf(LL_INFO, "PARTYMOD for MHPB %d.%d.%d\n", VERSION_NUMBER_MAJOR, VERSION_NUMBER_MINOR, VERSION_NUMBER_PATCH);

	log_printf(LL_INFO, "DIRECTORY: %s\n", executableDirectory);

#ifdef MEM_AUDIT
	initMemAudit();
#endif

	initEvents();
	initInput();

	log_printf(LL_INFO, "Patch Initialized\n");
}

void fatalError(const char *msg) {
	log_printf(LL_ERROR, "FATAL ERROR: %s\n", msg);
	createErrorMessageBox(msg);
	exit(1);
}

void quitGame() {
	

	exit(0);
}

void loadAudioConfig() {
	//uint32_t baseAddr = *((uint32_t *)0x539db4);

	uint16_t *sound_vol = 0x0053c12e;
	uint16_t *music_vol = 0x0053c130;

	sound_vol = 7;
	music_vol = 6;
}

void loadConfig() {
	loadAudioConfig();
	loadGfxSettings();
}

int WinYield() {
	int result = 0x75;

	handleEvents();

	return result;
}

void patchWindowAndInit() {
	//patchCall(0x00404f67, initPatch);
	patchByte(0x004051a0 + 6, 1);
	patchByte(0x004051aa + 6, 0);
	patchNop(0x00404f74, 119);
	patchNop(0x00404ffc, 39);
	patchCall(0x00404f74, initPatch);
	
	patchJmp(0x00405f90, WinYield);

	patchJmp(0x004049e0, loadConfig);
}

void patchModuleEventHandler(int module, uint32_t baseAddr) {
	switch(module) {
	case 0:
		patchJmp(baseAddr + 0x00024240, WinYield);
		break;
	case 1:
		patchJmp(baseAddr + 0x0007bf70, WinYield);
		break;
	case 2:
		patchJmp(baseAddr + 0x00038a70, WinYield);
		break;
	}
}

extern int currentModule = -1;

int (* _cdecl origLoadModule)(char *) = 0x004051e0;

int _cdecl loadModuleWrapper(char *name) {
	int result = origLoadModule(name);

	uint32_t hash = memhash(name, strlen(name));
	currentModule = -1;
	switch(hash) {
	case 0x9b7a3eb7:
		log_printf(LL_INFO, "loading frontend module\n");
		currentModule = 0;
		break;
	case 0x29ff8422:
		log_printf(LL_INFO, "loading source module\n");
		currentModule = 1;
		break;
	case 0x68366842:
		log_printf(LL_INFO, "loading editor module\n");
		currentModule = 2;
		break;
	default:
		log_printf(LL_INFO, "hash for %s is 0x%08x!!!\n", name, hash);
	}

	//0x539db4 = module offset
	void *baseAddr = *((void **)0x539db4);
	if (currentModule != -1) {
		log_printf(LL_INFO, "INSTALLING MODULE PATCHES\n");
		installModuleGfxPatches(currentModule, baseAddr);
		patchModuleEventHandler(currentModule, baseAddr);
		installModuleInputPatches(currentModule, baseAddr);
	}

	return result;
}

void installModuleWrapper() {
	patchCall(0x004010f3, loadModuleWrapper);
	patchCall(0x00401164, loadModuleWrapper);
	patchCall(0x004011c3, loadModuleWrapper);
	patchCall(0x0040124c, loadModuleWrapper);
}

__declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	// Perform actions based on the reason for calling.
	switch(fdwReason) { 
		case DLL_PROCESS_ATTACH:
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.

			// install patches
			patchWindowAndInit();
			installWindowPatches();
			//installInputPatches();
			installGfxPatches();
			//installMemPatches();
			//patchSaveOpen();
			//patchOptionsMenu();
			installModuleWrapper();

			//installAltMemManager();

			break;

		case DLL_THREAD_ATTACH:
			// Do thread-specific initialization.
			break;

		case DLL_THREAD_DETACH:
			// Do thread-specific cleanup.
			break;

		case DLL_PROCESS_DETACH:
			// Perform any necessary cleanup.
			break;
	}
	return TRUE;
}
