#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h> /* for ROOT_DEV */

#include <arch/segment.h>

int arch_cpu;			/* Processor type */
#ifdef CONFIG_ARCH_SIBO
extern long int basmem;
#endif

void setup_arch(seg_t *start, seg_t *end)
{
#ifdef CONFIG_COMPAQ_FAST

/*
 *	Switch COMPAQ Deskpro to high speed
 */

    outb_p(1,0xcf);

#endif

/*
 *	Fill in the MM numbers - really ought to be in mm not kernel ?
 */

/*
 *      This computes the 640K - _endbss
 */

#ifndef CONFIG_ARCH_SIBO

    *end = (seg_t)((setupw(0x2a) << 6) - RAM_REDUCE);

    /* XXX plac: free root ram disk */

    *start = kernel_ds;
    *start += ((unsigned int) (_endbss+15)) >> 4;

#else

    *end = (basmem)<<6;
    *start = kernel_ds;
    *start += (unsigned int) 0x1000;

#endif

    ROOT_DEV = setupw(0x1fc);

    arch_cpu = setupb(0x20);

}

/* Stubs for functions needed elsewhere */

#ifndef S_SPLINT_S
#asm
	export _hard_reset_now

_hard_reset_now:

	mov ax,#0x40		! No memory check on reboot
	mov ds, ax
	mov [0x72],#0x1234
	jmp #0xffff:0

#endasm
#endif
