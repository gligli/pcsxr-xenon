#include "config.h"

#include "psxcommon.h"
#include "debug.h"
#include "sio.h"
#include "misc.h"
//#include "cheat.h"

#include "gamecube_plugins.h"

#define NUM_PLUGINS 8

PluginTable plugins[NUM_PLUGINS] = {
	EMPTY_PLUGIN,
	SPU_XENON_PLUGIN,
	GPU_HW_PEOPS_PLUGIN,
	GPU_PEOPS_PLUGIN,
	CDRCIMG_PLUGIN,
	PAD1_PLUGIN,
	PAD2_PLUGIN,
	EMPTY_PLUGIN,
};

int SysInit() {
	if (EmuInit() == -1) return -1;

	LoadMcds(Config.Mcd1, Config.Mcd2);

	return 0;
}

void SysReset() {
	EmuReset();
}

void SysClose() {
	EmuShutdown();
}

void SysPrintf(const char *fmt, ...) {
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	printf(msg);

}

void SysMessage(const char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);
	printf(tmp);
}

static char *err = N_("Error Loading Symbol");
static int errval;

void *SysLoadLibrary(const char *lib) {
	int i;
	for (i = 0; i < NUM_PLUGINS; i++)
		if ((plugins[i].lib != NULL) && (!strcmp(lib, plugins[i].lib))) {
			return (void*)&plugins[i];
		}
	return NULL;
}

void *cdrcimg_get_sym(const char *sym);

void *SysLoadSym(void *lib, const char *sym) {
	PluginTable* plugin = (PluginTable*) lib;
	int i;
	for (i = 0; i < plugin->numSyms; i++) {
		if (plugin->syms[i].sym && !strcmp(sym, plugin->syms[i].sym)) {
			return plugin->syms[i].pntr;
		}
	}
	return NULL;
}

const char *SysLibError() {
	//printf("SysLibError\r\n");
	if (errval) {
		errval = 0;
		return err;
	}
	return NULL;
}

void SysCloseLibrary(void *lib) {
	// FreeLibrary((HINSTANCE) lib);
}

void SysUpdate() {

}

void SysRunGui() {

}
