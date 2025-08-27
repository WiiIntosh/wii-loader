/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.

Copyright (C) 2008, 2009	Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "utils.h"
#include "start.h"
#include "hollywood.h"
#include "sdhc.h"
#include "string.h"
#include "memory.h"
#include "gecko.h"
#include "ff.h"
#include "panic.h"
#include "powerpc_elf.h"
#include "irq.h"
#include "exception.h"
#include "crypto.h"
#include "nand.h"
#include "boot2.h"
#include "git_version.h"

#define PPC_BOOT_FILE "/openbios.elf"

FATFS fatfs;

// These defines are for the ARMCTRL regs
// See http://wiibrew.org/wiki/Hardware/IPC
#define		IPC_CTRL_Y1	0x01
#define		IPC_CTRL_X2	0x02
#define		IPC_CTRL_X1	0x04
#define		IPC_CTRL_Y2	0x08

// reset both flags (X* for ARM and Y* for PPC)
#define		IPC_CTRL_RESET	0x06

// IPC commands.
#define CMD_POWEROFF    0xCAFE0001
#define CMD_REBOOT      0xCAFE0002

u32 _main(void *base)
{
	FRESULT fres;
	int res;
	u32 vector;
	(void)base;

	gecko_init();
	gecko_printf("mini %s loading\n", git_version);

	gecko_printf("Initializing exceptions...\n");
	exception_initialize();
	gecko_printf("Configuring caches and MMU...\n");
	mem_initialize();

	gecko_printf("IOSflags: %08x %08x %08x\n",
		read32(0xffffff00), read32(0xffffff04), read32(0xffffff08));
	gecko_printf("          %08x %08x %08x\n",
		read32(0xffffff0c), read32(0xffffff10), read32(0xffffff14));

	irq_initialize();
//	irq_enable(IRQ_GPIO1B);
	irq_enable(IRQ_GPIO1);
	irq_enable(IRQ_RESET);
	irq_enable(IRQ_TIMER);
	irq_set_alarm(20, 1);
	gecko_printf("Interrupts initialized\n");

	crypto_initialize();
	gecko_printf("crypto support initialized\n");

	nand_initialize();
	gecko_printf("NAND initialized.\n");

	boot2_init();

	gecko_printf("Initializing IPC...\n");
	write32(HW_IPC_ARMMSG, 0);
	write32(HW_IPC_PPCMSG, 0);
	write32(HW_IPC_PPCCTRL, IPC_CTRL_RESET);
	write32(HW_IPC_ARMCTRL, IPC_CTRL_RESET);

	gecko_printf("Initializing SDHC...\n");
	sdhc_init();

	gecko_printf("Mounting SD...\n");
	fres = f_mount(0, &fatfs);

	if (read32(0x0d800190) & 2) {
		gecko_printf("GameCube compatibility mode detected...\n");
		vector = boot2_run(1, 0x101);
		goto shutdown;
	}

	if(fres != FR_OK) {
		gecko_printf("Error %d while trying to mount SD\n", fres);
		panic2(0, PANIC_MOUNT);
	}

	gecko_printf("Trying to boot:" PPC_BOOT_FILE "\n");

	res = powerpc_boot_file(PPC_BOOT_FILE);
	if(res < 0) {
		gecko_printf("Failed to boot PPC: %d\n", res);
		gecko_printf("Booting System Menu\n");
		vector = boot2_run(1, 2);
		goto shutdown;
	}

	//
	// Disable interrupts, no longer needed.
	//
	gecko_printf("Shutting down interrupts...\n");
	irq_shutdown();

	while (1) {
		//
		// Check for message sent flag.
		//
		u32 ctrl = read32(HW_IPC_ARMCTRL);
		if (!(ctrl & IPC_CTRL_X1)) {
			continue;
		}

		//
		// Read PowerPC's message and handle command.
		//
		u32 msg = read32(HW_IPC_PPCMSG);
		if (msg == CMD_POWEROFF) {
			write32(HW_GPIO1OUT, read32(HW_GPIO1OUT) | HW_GPIO1_SHUTDOWN);
		} else if (msg == CMD_REBOOT) {
			write32(HW_RESETS, read32(HW_RESETS) & ~(HW_RESETS_RSTBINB));
		}

		// writeback ctrl value to reset IPC
		write32(HW_IPC_ARMCTRL, ctrl);
	}

shutdown:
	gecko_printf("Shutting down interrupts...\n");
	irq_shutdown();
	gecko_printf("Shutting down caches and MMU...\n");
	mem_shutdown();

	gecko_printf("Vectoring to 0x%08x...\n", vector);
	return vector;
}

