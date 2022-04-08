#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#include "host.h"
#include "basic.h"

unsigned char mem[MEMORY_SIZE];
static unsigned char tokenBuf[TOKEN_BUF_SIZE];

static FILE *infile;
static FILE *outfile;

void host_cls() {
	fprintf(outfile, "\033[H\033[2J");
}

void host_moveCursor(int x, int y) {
	fprintf(outfile, "\033[%d;%dH", y, x);
}

void host_showBuffer() {
	fflush(outfile);
}

void host_outputString(char *str) {
	fprintf(outfile, "%s", str);
}

void host_outputChar(char c) {
	fprintf(outfile, "%c", c);
}

void host_outputLong(long num) {
	fprintf(outfile, "%ld", num);
}

// for float testing compatibility, use same FP formatting routines on host for now
// floats have approx 7 sig figs, 15 for double
#ifdef __ia6__
__STDIO_PRINT_FLOATS;		// link in libc printf float support

char *host_floatToStr(float f, char *buf) {
	sprintf(buf, "%.*g", MATH_PRECISION, (double)f);
    return buf;
}
#else
void __fp_print_func(double val, int style, int preci, char * ptmp);
char *ecvt(double val, int ndig, int *pdecpt, int *psign);
char *fcvt(double val, int ndig, int *pdecpt, int *psign);

char *host_floatToStr(float f, char *buf) {
	__fp_print_func(f, 'g', MATH_PRECISION, buf);
    return buf;
}
#endif

void host_outputFloat(float f) {
    char buf[32];
    host_outputString(host_floatToStr(f, buf));
}

void host_newLine() {
	fprintf(outfile, "\n");
}

char *host_readLine() {
	static char buf[80];

	if (!fgets(buf, sizeof(buf), infile))
		return NULL;
	buf[strlen(buf)-1] = 0;
	return buf;
}

char host_getKey() {
#if 0
    char c = inkeyChar;
    inkeyChar = 0;
    if (c >= 32 && c <= 126)
        return c;
#endif
    return 0;
}

int host_ESCPressed() {
#if 0
    while (keyboard.available()) {
        // read the next key
        inkeyChar = keyboard.read();
        if (inkeyChar == PS2_ESC)
            return true;
    }
#endif
    return false;
}

void host_outputFreeMem(unsigned int val)
{
	printf("%u bytes free\n", val);
}

/* LONG_MAX is not exact as a double, yet LONG_MAX + 1 is */
#define LONG_MAX_P1 ((LONG_MAX/2 + 1) * 2.0)

/* small ELKS floor function using 32-bit longs */
float host_floor(float x)
{
  if (x >= 0.0) {
    if (x < LONG_MAX_P1) {
      return (float)(long)x;
    }
    return x;
  } else if (x < 0.0) {
    if (x >= LONG_MIN) {
      long ix = (long) x;
      return (ix == x) ? x : (float)(ix-1);
    }
    return x;
  }
  return x; // NAN
}

void host_sleep(long ms) {
    usleep(ms * 1000);
}

#ifdef __ia16__
#include <linuxmt/config.h>
#endif
#ifdef CONFIG_ARCH_8018X
/**
 * NOTE: This only works on MY 80C188EB board, needs to be more
 * generalized using the 8018x's GPIOs instead!
 */
#include "arch/io.h"

static uint8_t the_leds = 0xfe;
void host_digitalWrite(int pin,int state) {
    if (pin > 7) {
        return;
    }

    uint8_t new_state = 1 << pin;
    if (state == 0) {
        the_leds |= new_state;
    } else {
        the_leds &= ~new_state;
    }
    /* there's a latch on port 0x2002 where 8 leds are connected to */
    outb(the_leds, 0x2002);
}

int host_digitalRead(int pin) {
    if (pin > 7) {
        if (pin == 69) {
            /* buffer on port 0x2003, bit #3 is the switch */
            return inb(0x2003) & (1 << 3) ? 1 : 0;
        }
        return 0;
    }

    /* there's a buffer on port 0x2001 where an 8 switches dip switch is conncted to */
    uint8_t b = inb(0x2001);
    uint8_t bit = 1 << pin;

    if (b & bit) {
        return 1;
    }
	return 0;
}
#else

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}
#endif

int host_analogRead(int pin) {
    //return analogRead(pin);
	return 0;
}

void host_pinMode(int pin,int mode) {
    //pinMode(pin, mode);
}

#if DISK_FUNCTIONS

#include <dirent.h>

int host_directoryListing() {
	DIR		*dirp;
	struct	dirent	*dp;

    dirp = opendir(".");
    if (dirp == NULL)
		return ERROR_FILE_ERROR;

    while ((dp = readdir(dirp)) != NULL) {
        char *dot = strrchr(dp->d_name, '.'); /* Find last '.', if there is one */
        if (dot && (strcasecmp(dot, ".bas") == 0)) {
            host_outputString(dp->d_name);
            host_newLine();
        }
    }
    closedir(dirp);

	return ERROR_NONE;
}

int host_removeFile(char *fileName) {
	char file[MAX_PATH_LEN+5];

	strcpy(file, fileName);
	if (!strstr(file, ".bas"))
		strcat(file, ".bas");

    if (unlink(file) < 0)
		return ERROR_FILE_ERROR;
    return ERROR_NONE;
}

int host_saveProgramToFile(char *fileName, int autoexec) {
	char file[MAX_PATH_LEN+5];

	strcpy(file, fileName);
	if (!strstr(file, ".bas"))
		strcat(file, ".bas");
	FILE* fh = fopen(file, "w");

	if (!fh)
		return ERROR_FILE_ERROR;

	outfile = fh;
	listProg(0, 0);
	if (autoexec)
		fprintf(outfile, "RUN\n");
	fclose(outfile);
	outfile = stdout;

    return ERROR_NONE;
}
#endif

// BASIC

int loop(int showOK) {
    int ret = ERROR_NONE;

    lineNumber = 0;
    char *input = host_readLine();
    if (!input) return ERROR_EOF;

    if (!strcmp(input, "mem")) {
        host_outputFreeMem(sysVARSTART - sysPROGEND);
        return ERROR_NONE;
    }
    ret = tokenize((unsigned char*)input, tokenBuf, TOKEN_BUF_SIZE);

    /* execute the token buffer */
    if (ret == ERROR_NONE)
        ret = processInput(tokenBuf);

    if (ret != ERROR_NONE) {
        host_newLine();
        if (lineNumber != 0) {
            host_outputLong(lineNumber);
            host_outputChar('-');
        }
        printf("%s\n", errorTable[ret]);
    } else if (showOK)
        printf("Ok\n\n");
    return ret;
}

int host_loadProgramFromFile(char *fileName) {
	int err;
	char file[MAX_PATH_LEN+5];

	strcpy(file, fileName);
	if (!strstr(file, ".bas"))
		strcat(file, ".bas");

	infile = fopen(file, "r");
	if (!infile)
		return ERROR_FILE_ERROR;

	while ((err = loop(0)) == ERROR_NONE)
		continue;

	fclose(infile);
	infile = stdin;
	if (err == ERROR_EOF) err = ERROR_NONE;
	return err;
}

int main(int ac, char **av) {
	infile = stdin;
	outfile = stdout;

	reset();

	printf("Sinclair BASIC\n");
	host_outputFreeMem(sysVARSTART - sysPROGEND);
	printf("\n");

	if (ac > 1) {
		int err = host_loadProgramFromFile(av[1]);
		if (err != ERROR_NONE) {
			printf("Can't load %s\n", av[1]);
			exit(1);
		} else printf("Ok\n\n");
	}
    
	for(;;)
		if (loop(1) == ERROR_EOF)
			break;
	return 0;
}
