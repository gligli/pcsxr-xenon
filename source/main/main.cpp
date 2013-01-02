#ifndef USE_GUI

#include <xenos/xenos.h>
#include <xenos/xe.h>
#include <xenon_sound/sound.h>
#include <diskio/ata.h>
#include <ppc/cache.h>
#include <ppc/timebase.h>
#include <pci/io.h>
#include <input/input.h>
#include <xenon_smc/xenon_smc.h>
#include <console/console.h>
#include <xenon_soc/xenon_power.h>
#include <usb/usbmain.h>
#include <ppc/timebase.h>
#include <sys/iosupport.h>


#define BP {printf("[Breakpoint] in function %s, line %d, file %s\n",__FUNCTION__,__LINE__,__FILE__);getch();}
#define TR {printf("[Trace] in function %s, line %d, file %s\n",__FUNCTION__,__LINE__,__FILE__);}

#include "config.h"
#include "r3000a.h"
#include "psxcommon.h"
#include "debug.h"
#include "sio.h"
#include "misc.h"
//#include "cheat.h"
#include <stdio.h>
#include <console/console.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include "libwiigui/gui.h"
#include <libfat/fat.h>

#include <libntfs/ntfs.h>
#include <libxtaf/xtaf.h>

#include "gamecube_plugins.h"

extern "C" void httpd_start(void);

extern PluginTable plugins[];

// #define cdfile "uda:/psxisos/BREATH_OF_FIRE3.BIN"
#define cdfile "uda:/psxisos/MEGAMAN8.BIN"
// #define cdfile "uda:/psxisos/RPG Maker (U).bin"
#define cdfile "uda:/psxisos/Vagrant Story[NTSC-U].iso"
// #define cdfile "uda:/psx.iso"
// #define cdfile "uda:/psxisos/Super Street Fighter Collection.bin"
#define cdfile "uda:/psxisos/Strider 2 (USA)/Strider 2 (USA).bin"
#define cdfile "uda:/psxisos/Street Fighter Alpha - Warriors' Dreams (USA)/Street Fighter Alpha - Warriors' Dreams (USA) (Track 01).bin"
// #define cdfile "uda:/psxisos/Street Fighter Alpha 2 (USA)/Street Fighter Alpha 2 (USA) (Track 1).bin"
#define cdfile "uda:/psxisos/odd.iso"
#define cdfile "uda:/psxisos/MEGAMAN8.BIN"
#define cdfile "uda:/psxisos/PSX_Tekken_3_(NTSC).Iso"

//#define cdfile "uda:/psxisos/Street Fighter Alpha - Warriors' Dreams (USA)/Street Fighter Alpha - Warriors' Dreams (USA) (Track 01).bin"
//#define cdfile "uda:/psxisos/Soul Blade (USA) (v1.1).bin"


//#define cdfile "uda:/psxisos/Strider 2 (USA)/Strider 2 (USA).bin"
//#define cdfile "uda:/psxisos/CTR - Crash Team Racing (USA).bin"
//#define cdfile "uda:/psxisos/Strider 2 (USA)/Strider 2 (USA).bin"
//#define cdfile "uda:/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"

/*
#define cdfile "uda:/psxisos/Strider 2 (USA)/Strider 2 (USA).bin"

#define cdfile "uda:/psxisos/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"

#define cdfile "uda:/psxisos/Soul Blade (USA) (v1.1).bin"
#define cdfile "uda:/psxisos/Final Fantasy VII (USA)/Final Fantasy VII (USA) (Disc 1).bin"


#define cdfile "uda:/psxisos/Crash Bandicoot - Warped (USA).bin"
#define cdfile  "uda:/psxisos/Final Fantasy VII (F)/Final Fantasy VII (F) (Disc 1) [SCES-00868].bin"

#define cdfile "uda:/psxisos/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"
 */
/*
#define cdfile "uda:/psxisos/CTR - Crash Team Racing (USA).bin"
 */
//#define cdfile "uda:/psxisos/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"
//#define cdfile "uda:/psxisos/CTR - Crash Team Racing (USA).bin"
#define cdfile "uda:/psxisos/Final Fantasy VII (USA)/Final Fantasy VII (USA) (Disc 1).bin"
//#define cdfile "uda:/psxisos/Resident Evil - Director's Cut - Dual Shock Ver. (USA)/Resident Evil - Director's Cut - Dual Shock Ver. (USA) (Track 1).bin"
//#define cdfile "uda:/psxiso.iso"
//#define cdfile "uda:/psxisos/Gran Turismo 2 (USA) (v1.0) (Simulation Mode)/Gran Turismo 2 (USA) (v1.0) (Simulation Mode).bin"

#define cdfile "uda:/psxisos/Street Fighter Alpha 2 (USA)/Street Fighter Alpha 2 (USA) (Track 1).bin"
//#define cdfile "uda:/psxiso.iso"
//#define cdfile "uda:/psxisos/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"
//#define cdfile  "uda:/psxisos/Final Fantasy VII (F)/Final Fantasy VII (F) (Disc 1) [SCES-00868].bin"
//#define cdfile "uda:/psxisos/CTR - Crash Team Racing (USA).bin"
//#define cdfile "uda:/psxisos/Crash Bandicoot - Warped (USA).bin"
//#define cdfile "uda:/psxisos/WipEout XL (USA)/WipEout XL (USA) (Track 01).bin"
//#define cdfile "uda:/psxisos/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"
//#define cdfile "uda:/psxiso.iso"
//#define cdfile "uda:/psxisos/Rayman (USA)/Rayman (USA) (Track 01).bin"
//#define cdfile "uda:/psxisos/Darkstalkers 3 (USA)/Darkstalkers 3 (USA).bin"
//#define cdfile "uda:/psxisos/Gran Turismo 2 (USA) (v1.0) (Simulation Mode)/Gran Turismo 2 (USA) (v1.0) (Simulation Mode).bin"

//#define cdfile "uda:/psxisos/Street Fighter Alpha - Warriors' Dreams (USA)/Street Fighter Alpha - Warriors' Dreams (USA) (Track 01).bin"
//#define cdfile "uda:/psxisos/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin"


//#define cdfile "uda:/psxisos/CTR - Crash Team Racing (USA).bin"
//#define cdfile "uda:/psxisos/R4 - Ridge Racer Type 4 (USA)/R4 - Ridge Racer Type 4 (USA).bin"

// #define cdfile "uda:/psxisos/Metal Gear Solid (France) (Disc 1)/Metal Gear Solid (France) (Disc 1).bin"
//#define cdfile "uda:/psxisos/Gran Turismo 2 (USA) (v1.0) (Simulation Mode)/Gran Turismo 2 (USA) (v1.0) (Simulation Mode).bin"
//#define cdfile "sda:/hdd1/xenon/psx/CTR - Crash Team Racing (USA)/CTR - Crash Team Racing (USA).bin.Z"
//#define cdfile "sda:/hdd1/xenon/psx/Tekken 3 (USA)/Tekken 3 (USA) (Track 1).bin.Z"
//#define cdfile "sda:/hdd1/xenon/psx/R4 - Ridge Racer Type 4 (USA).bin"
#define cdfile "uda:/CTR - Crash Team Racing (USA).bin"
//#define cdfile "uda:/Street Fighter Alpha - Warriors' Dreams (USA) (Track 01).bin"
//#define cdfile "uda:/Tekken 3 (USA) (Track 1).bin.Z"
//
//#define cdfile "uda:/R4 - Ridge Racer Type 4 (USA).bin.Z"
//#define cdfile "uda:/Final Fantasy VII (USA) (Disc 1).bin"
#define cdfile "uda:/Tekken 3 (USA) (Track 1).bin.Z"

//#define cdfile "uda:/CTR - Crash Team Racing (USA).bin.Z"

#define cdfile "uda:/Resident Evil (USA).iso.Z"
#define cdfile "uda:/0cimg/medievil-scus-94227-/Medievil.bin.Z"


#define cdfile "uda:/Street Fighter Alpha 2 [SLUS-00258].img.Z"
#define cdfile "uda:/ff9_patched.bin"
#define cdfile "usb:/Final Fantasy IX (France) (Disc 1).bin.Z"

#define cdfile "uda:/pcsxr/iso/Final Fantasy IX (France) (Disc 1).bin"
//#define cdfile "uda:/pcsxr/iso/Crash Bandicoot 3.bin"

//#define cdfile "uda:/pcsxr/iso/medievil2.img.Z"

#define cdfile "sda0:/devkit/pcsxr/Castlevanina - SOTN.bin"

//#define cdfile "uda0:/tekken3.bin"


//#define cdfile "uda:/pcsxr/iso/sfa.bin"

//#define cdfile "uda:/pcsxr/iso/toshiden2.img"

void printConfigInfo() {

}

static void findDevices() {
	for (int i = 3; i < STD_MAX; i++) {
		if (devoptab_list[i]->structSize) {
			printf("Device found: %s:/\n", devoptab_list[i]->name);
		}
	}
}

void buffer_dump(uint8_t * buf, int size) {
	int i = 0;
	TR;
	for (i = 0; i < size; i++) {

		printf("%02x ", buf[i]);
	}
	printf("\r\n");
}

uint8_t * xtaf_buff();

void SetIso(const char * fname) {
	FILE *fd = fopen(fname, "rb");
	if (fd == NULL) {
		printf("Error loading %s\r\n", fname);
		return;
	}
	uint8_t header[0x10];
	int n = fread(header, 0x10, 1, fd);
	printf("n : %d\r\n", n);

	buffer_dump(header, 0x10);

	if (header[0] == 0x78 && header[1] == 0xDA) {
		printf("Use CDRCIMG for  %s\r\n", fname);
		strcpy(Config.Cdr, "CDRCIMG");
		cdrcimg_set_fname(fname);
	} else {
		SetIsoFile(fname);
	}

	fclose(fd);
}
extern "C" {
	void useSoftGpu();
	void useHwGpu();
}


extern "C" DISC_INTERFACE usb2mass_ops_0;

extern "C" void init_miss();
int main() {

	printf("main\n");

	xenos_init(VIDEO_MODE_HDMI_720P);
	xenon_make_it_faster(XENON_SPEED_FULL);

	xenon_sound_init();
	//xenos_init(VIDEO_MODE_YUV_720P);
	//console_init();
	usb_init();
	usb_do_poll();
	xenon_ata_init();
	xenon_atapi_init();

	//fatInitDefault();
	
	//char mount[10];
	//sprintf(mount, "uda0");
	//fatMount(mount, &usb2mass_ops_0, 0, 2, 64);
	
	ntfs_md *mounts;
	//ntfsMountAll (&mounts, NTFS_READ_ONLY);
	
	
	XTAFMount();
	
	findDevices();
	
	init_miss();
	
	time_t rawtime;
	time ( &rawtime );
	printf ( "The current local time is: %s", ctime (&rawtime) );
	
	/*

	 */
	memset(&Config, 0, sizeof (PcsxConfig));

	//    network_init();
	//    network_print_config();

	//console_close();

	xenon_smc_start_bootanim(); // tell me that telnet or http are ready

	// telnet_console_init();
	// mdelay(5000);

	//httpd_start();

	// uart speed patch 115200 - jtag/freeboot
	// *(volatile uint32_t*)(0xea001000+0x1c) = 0xe6010000;

	//memset(&Config, 0, sizeof (PcsxConfig));
	strcpy(Config.Net, "Disabled");
	strcpy(Config.Cdr, "CDR");
	strcpy(Config.Gpu, "GPU");
	strcpy(Config.Spu, "SPU");
	strcpy(Config.Pad1, "PAD1");
	strcpy(Config.Pad2, "PAD2");

	strcpy(Config.Bios, "SCPH1001.BIN"); // Use actual BIOS
	//strcpy(Config.Bios, "scph7502.bin"); // Use actual BIOS
	//strcpy(Config.Bios, "HLE"); // Use HLE
	strcpy(Config.BiosDir, "sda0:/devkit/pcsxr/bios");
	strcpy(Config.PatchesDir, "sda0:/devkit/pcsxr/patches_/");

	Config.PsxAuto = 1; // autodetect system
	
	Config.Cpu = CPU_DYNAREC;
	//Config.Cpu =  CPU_INTERPRETER;

	strcpy(Config.Mcd1, "sda0:/devkit/pcsxr/memcards/card1.mcd");
	strcpy(Config.Mcd2, "sda0:/devkit/pcsxr/memcards/card2.mcd");

	// useSoftGpu();
	/*
		strcpy(Config.Mcd1, "sda:/hdd1/xenon/memcards/card1.mcd");
		strcpy(Config.Mcd2, "sda:/hdd1/xenon/memcards/card2.mcd");
	 */

	//InitVideo();

	SetIso(cdfile);
	if (LoadPlugins() == 0) {
		if (OpenPlugins() == 0) {
			if (SysInit() == -1) {
				printf("SysInit() Error!\n");
				return -1;
			}

			SysReset();
			// Check for hle ...
			if (Config.HLE == 1) {
				printf("Can't continue ... bios not found ...\r\n");
				//exit(0);
			}

			CheckCdrom();
			LoadCdrom();

			psxCpu->Execute();
		}
	}

	printf("Pcsx exit ...\r\n");
	return 0;
}

void cpuReset() {
	EmuReset();
}

#include "gui.h"
SPU_Config SpuConfig;
HW_GPU_Config HwGpuConfig;

extern "C" void systemPoll() {
	// network_poll();
}

#endif
