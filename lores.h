/* lores.h v0.20250226a - a minimalistic game framework portable across Linux and Windows

License: zlib

Copyright (c) 2025 by Gerald Franz, gerald.franz@eludi.net

This software is provided 'as-is', without any express or implied warranty. In no event will the
authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial
applications, and to alter it and redistribute it freely, subject to the following restrictions:

(1) The origin of this software must not be misrepresented; you must not claim that you wrote the
	original software. If you use this software in a product, an acknowledgment in the product
	documentation would be appreciated but is not required.
(2) Altered source versions must be plainly marked as such, and must not be misrepresented as being
	the original software.
(3) This notice may not be removed or altered from any source distribution.
*/

#ifndef _LORES_H
#define _LORES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define KEY_BACKSPACE 8
#define KEY_TAB 9
#define KEY_ENTER 13
#define KEY_ESCAPE 27
#define KEY_UNKNOWN -1
#define KEY_UP -72
#define KEY_DOWN -80
#define KEY_RIGHT -77
#define KEY_LEFT -75
#define KEY_INSERT -82
#define KEY_DELETE -83
#define KEY_PAGEUP -73 
#define KEY_PAGEDOWN -81
#define KEY_HOME -71
#define KEY_END -79
#define KEY_F1 -59
#define KEY_F2 -60
#define KEY_F3 -61
#define KEY_F4 -62
#define KEY_F5 -63
#define KEY_F6 -64
#define KEY_F7 -65
#define KEY_F8 -66
#define KEY_F9 -67
#define KEY_F10 -68
#define KEY_F11 -133
#define KEY_F12 -134
// todo what about modifier keys?

#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_LIGHT_GRAY 7
#define COLOR_GRAY 8
#define COLOR_LIGHT_RED 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_YELLOW 11
#define COLOR_LIGHT_BLUE 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_CYAN 14
#define COLOR_WHITE 15
#define COLOR_TRANSPARENT 255

/// portable console input output similar to conio.h

// Call shutdownscr() (or register it with atexit) before normal process exit.
void initscr();
void shutdownscr();
void scrsize(int* width, int* height);
void echoon();
void echooff();
int kbhit();
// Linux escape-sequence inter-byte timeout; override before including this header.
#ifndef LORES_KEY_TIMEOUT_MS
#define LORES_KEY_TIMEOUT_MS 50
#endif
// On Linux, do not mix these keyboard functions with stdio reads from stdin.
int getkey();

static inline void clrscr() { printf("\033[H\033[J"); }
static inline void clreol() { printf("\033[K"); }
static inline void gotoxy(int x,int y) { printf("\033[%d;%df", y, x); }
static inline void storexy() { printf("\0337"); }
static inline void restorexy() { printf("\0338"); }
static inline void cursoroff() { printf("\033[?25l"); }
static inline void cursoron() { printf("\033[?25h"); }
static inline void textcolor(int code) { printf("\033[38;5;%dm", code); }
static inline void textbackground(int code) { printf("\033[48;5;%dm", code); }
static inline void colorReset() { printf("\033[0m"); }

//------------------------------------------------------------------

void delay(uint32_t duration_ms);
// Define LORES_NO_AUDIO to make sound() a no-op without audio dependencies.
void sound(uint32_t frequency, uint32_t duration_ms);
uint64_t timestamp();
// Uniform integer in [0, upperBound); an empty range returns zero.
static inline uint32_t randi(uint32_t upperBound) {
	if (!upperBound) return 0;
	const uint64_t base = (uint64_t)RAND_MAX + 1;
	uint64_t value, range, limit;
	do {
		value = 0;
		range = 1;
		do {
			value = value * base + (unsigned int)rand();
			range *= base;
		} while (range < upperBound);
		limit = range - range % upperBound;
	} while (value >= limit);
	return (uint32_t)(value % upperBound);
}

static inline float randf() { return randi(16777216) / 16777216.0f; }

//------------------------------------------------------------------

/// blockbuf low resolution textmode graphics

typedef struct {
	uint16_t w, h;
	uint8_t buf[];
} BlockBuf;

BlockBuf* BlockBufCreate(uint16_t w, uint16_t h);
void BlockBufSet(BlockBuf* bb, uint16_t x, uint16_t y, uint8_t color);
void BlockBufBlt(BlockBuf* bb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t pattern[]);
void BlockBufFill(BlockBuf* bb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
void BlockBufOut(const BlockBuf* bb, uint16_t x, uint16_t y);

#define FLIP_X 1
#define FLIP_Y 2
#define FLIP_D 4

// Returns NULL on allocation failure or when FLIP_D is used on a nonsquare sprite.
uint8_t* BlockBufDecode(const char data[], uint16_t w, uint16_t h, const char* colorMap, int flags);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _LORES_H

//-------------------------------------------------------------------------------------------------
#ifdef LORES_H_IMPLEMENTATION
#  include <time.h>

#ifndef LORES_NO_AUDIO
typedef struct { uint32_t frequency, duration_ms; } LoresSoundArgs;
#endif

#if defined __WIN32__ || defined WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <conio.h>
#  ifndef LORES_NO_AUDIO
#    include <process.h>
#  endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#  define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif

static int lores_screen_active;
static DWORD lores_input_mode, lores_output_mode;
static int lores_have_input_mode, lores_have_output_mode;
static UINT lores_output_codepage;
static CONSOLE_CURSOR_INFO lores_cursor_info;
static CONSOLE_SCREEN_BUFFER_INFO lores_screen_info;
static int lores_have_cursor_info, lores_have_screen_info;

void initscr() {
	if (lores_screen_active) return;
	srand(time(NULL));
	HANDLE sin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE sout = GetStdHandle(STD_OUTPUT_HANDLE);
	lores_have_input_mode = GetConsoleMode(sin, &lores_input_mode);
	lores_have_output_mode = GetConsoleMode(sout, &lores_output_mode);
	lores_have_cursor_info = GetConsoleCursorInfo(sout, &lores_cursor_info);
	lores_have_screen_info = GetConsoleScreenBufferInfo(sout, &lores_screen_info);
	lores_output_codepage = GetConsoleOutputCP();
	lores_screen_active = 1;
	if (lores_have_output_mode) {
		SetConsoleMode(sout, lores_output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		SetConsoleOutputCP(CP_UTF8);
	}
}

void shutdownscr() {
	if (!lores_screen_active) return;
	fflush(stdout);
	HANDLE sin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE sout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (lores_have_screen_info) SetConsoleTextAttribute(sout, lores_screen_info.wAttributes);
	if (lores_have_cursor_info) SetConsoleCursorInfo(sout, &lores_cursor_info);
	if (lores_have_input_mode) SetConsoleMode(sin, lores_input_mode);
	if (lores_have_output_mode) SetConsoleMode(sout, lores_output_mode);
	if (lores_output_codepage) SetConsoleOutputCP(lores_output_codepage);
	lores_screen_active = 0;
}

void echooff() {
	DWORD inMode = 0;
	HANDLE sin = GetStdHandle(STD_INPUT_HANDLE);
	if(sin == INVALID_HANDLE_VALUE || !GetConsoleMode(sin, &inMode))
		exit(GetLastError());
	inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	if(!SetConsoleMode(sin, inMode))
		exit(GetLastError());
}

void echoon() {
	DWORD inMode = 0;
	HANDLE sin = GetStdHandle(STD_INPUT_HANDLE);
	if(sin == INVALID_HANDLE_VALUE || !GetConsoleMode(sin, &inMode))
		exit(GetLastError());
	inMode |= (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	if(!SetConsoleMode(sin, inMode))
		exit(GetLastError());
}

int getkey() {
	int c = getch();
	if(c==0xE0 || c==0) // getch supplies the second special-key byte
		c = -getch();
	return c;
}

void delay(uint32_t duration_ms) {
	Sleep(duration_ms);
}

#ifndef LORES_NO_AUDIO
static void sound_thread(void *args) {
	const LoresSoundArgs params_arg = *(LoresSoundArgs*)args;
	free(args);
	const uint32_t frequency = params_arg.frequency;
	const uint32_t duration_ms = params_arg.duration_ms;
	Beep(frequency, duration_ms);
	_endthread();
}

void sound(uint32_t frequency, uint32_t duration_ms) {
	LoresSoundArgs* args = (LoresSoundArgs*)malloc(sizeof(*args));
	if (!args) return;
	args->frequency = frequency;
	args->duration_ms = duration_ms;
	if (_beginthread(sound_thread, 0, args) == (uintptr_t)-1)
		free(args);
}

#endif // !LORES_NO_AUDIO

#else
#  include <unistd.h>
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <poll.h>
#  include <errno.h>
#  ifndef LORES_NO_AUDIO
#  ifdef __cplusplus
extern "C" {
#  endif
#  include <alsa/asoundlib.h>
#  ifdef __cplusplus
}
#  endif
#  include <math.h>
#  include <pthread.h>
#  endif // !LORES_NO_AUDIO

static int lores_screen_active, lores_have_termios, lores_terminal_output;
static struct termios lores_original_termios;
static int lores_pending_key = -1;

void initscr() {
	if (lores_screen_active) return;
	srand(time(NULL));
	lores_have_termios = tcgetattr(STDIN_FILENO, &lores_original_termios) == 0;
	lores_terminal_output = isatty(STDOUT_FILENO);
	lores_screen_active = 1;
	if (lores_terminal_output) {
		// Save cursor visibility where DEC private-mode save/restore is supported.
		fputs("\033[?25s", stdout);
		fflush(stdout);
	}
}

void shutdownscr() {
	if (!lores_screen_active) return;
	if (lores_have_termios)
		tcsetattr(STDIN_FILENO, TCSANOW, &lores_original_termios);
	if (lores_terminal_output) {
		// Show the cursor as a fallback, then restore its saved visibility.
		fputs("\033[0m\033[?25h\033[?25r", stdout);
		fflush(stdout);
	}
	lores_pending_key = -1;
	lores_screen_active = 0;
}

void echooff() {
	struct termios t;
	if (tcgetattr(STDIN_FILENO, &t) != 0) return;
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void echoon() {
	struct termios t;
	if (tcgetattr(STDIN_FILENO, &t) != 0) return;
	t.c_lflag |= ICANON | ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Keyboard input uses the descriptor directly; do not mix it with stdio reads.
static int lores_read_key_byte(int timeout_ms) {
	if (lores_pending_key >= 0) {
		int ch = lores_pending_key;
		lores_pending_key = -1;
		return ch;
	}
	struct pollfd fd = { STDIN_FILENO, POLLIN, 0 };
	int ready;
	do { ready = poll(&fd, 1, timeout_ms); } while (ready < 0 && errno == EINTR);
	if (ready <= 0) return -1;
	unsigned char ch;
	ssize_t count;
	do { count = read(STDIN_FILENO, &ch, 1); } while (count < 0 && errno == EINTR);
	return count == 1 ? ch : -1;
}

static int lores_key_mode(struct termios *saved) {
	if (tcgetattr(STDIN_FILENO, saved) != 0) return 0;
	struct termios raw = *saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	return tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
}

int kbhit() {
	if (lores_pending_key >= 0) return 1;
	struct termios saved;
	int restore = lores_key_mode(&saved);
	lores_pending_key = lores_read_key_byte(0);
	if (restore) tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	return lores_pending_key >= 0;
}

static int lores_escape_key(void) {
	int ch = lores_read_key_byte(LORES_KEY_TIMEOUT_MS);
	if (ch < 0) return KEY_ESCAPE;
	if (ch != '[' && ch != 'O') {
		lores_pending_key = ch;
		return KEY_ESCAPE;
	}
	const int csi = ch == '[';
	int number = 0, first_parameter = 1;
	for (int n = 0; n < 32; ++n) {
		ch = lores_read_key_byte(LORES_KEY_TIMEOUT_MS);
		if (ch < 0) return KEY_UNKNOWN;
		if (csi && n == 0 && ch == '[') { // Linux console F1-F5
			ch = lores_read_key_byte(LORES_KEY_TIMEOUT_MS);
			return ch >= 'A' && ch <= 'E' ? KEY_F1 - (ch - 'A') : KEY_UNKNOWN;
		}
		if (ch >= '0' && ch <= '9') {
			if (first_parameter) number = number < 1000 ? number * 10 + ch - '0' : 1000;
			continue;
		}
		if (ch == ';') { first_parameter = 0; continue; }
		if (ch >= 0x20 && ch <= 0x3f) continue;
		switch (ch) {
		case 'A': return KEY_UP;
		case 'B': return KEY_DOWN;
		case 'C': return KEY_RIGHT;
		case 'D': return KEY_LEFT;
		case 'H': return KEY_HOME;
		case 'F': return KEY_END;
		case 'P': return KEY_F1;
		case 'Q': return KEY_F2;
		case 'R': return KEY_F3;
		case 'S': return KEY_F4;
		case '~':
			switch (number) {
			case 1: case 7: return KEY_HOME;
			case 2: return KEY_INSERT;
			case 3: return KEY_DELETE;
			case 4: case 8: return KEY_END;
			case 5: return KEY_PAGEUP;
			case 6: return KEY_PAGEDOWN;
			case 11: return KEY_F1;
			case 12: return KEY_F2;
			case 13: return KEY_F3;
			case 14: return KEY_F4;
			case 15: return KEY_F5;
			case 17: return KEY_F6;
			case 18: return KEY_F7;
			case 19: return KEY_F8;
			case 20: return KEY_F9;
			case 21: return KEY_F10;
			case 23: return KEY_F11;
			case 24: return KEY_F12;
			}
		}
		return KEY_UNKNOWN;
	}
	return KEY_UNKNOWN;
}

int getkey() {
	struct termios saved;
	int restore = lores_key_mode(&saved);
	int ch = lores_read_key_byte(-1);
	if (ch == '\n') ch = KEY_ENTER;
	else if (ch == 127) ch = KEY_BACKSPACE;
	else if (ch == KEY_ESCAPE) ch = lores_escape_key();
	else if (ch == 195) { // Preserve the existing two-byte Latin-1 key mapping.
		int next = lores_read_key_byte(LORES_KEY_TIMEOUT_MS);
		if (next >= 128 && next <= 191) ch = next;
		else if (next >= 0) lores_pending_key = next;
	}
	if (restore) tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	return ch;
}

void delay(uint32_t duration_ms) {
	usleep(duration_ms*1000);
}


#ifndef LORES_NO_AUDIO
static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;
static int sound_unavailable = 0;

static void sound_ignore_alsa_error(const char *file, int line, const char *func,
	int err, const char *fmt, va_list args) {
	(void)file; (void)line; (void)func; (void)err; (void)fmt; (void)args;
}

// Called with sound_mutex held, so concurrent failures warn only once.
static void sound_disable(void) {
	if (!sound_unavailable) {
		sound_unavailable = 1;
		fprintf(stderr, "lores: warning: audio unavailable; sound disabled\n");
	}
}

static void* sound_thread(void *args) {
	const LoresSoundArgs params_arg = *(LoresSoundArgs*)args;
	free(args);
	const uint32_t frequency = params_arg.frequency;
	const uint32_t duration_ms = params_arg.duration_ms;

	snd_pcm_t *handle = NULL;
	snd_pcm_hw_params_t *params;
	int dir = 0;
	unsigned int rate = 44100;
	snd_pcm_uframes_t frames = 32;

	pthread_mutex_lock(&sound_mutex);
	if (sound_unavailable) {
		pthread_mutex_unlock(&sound_mutex);
		return NULL;
	}
	// Silence ALSA only in this worker, preserving the caller's error handler.
	snd_local_error_handler_t old_handler = snd_lib_error_set_local(sound_ignore_alsa_error);
	snd_pcm_hw_params_alloca(&params);
	if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0 ||
		snd_pcm_hw_params_any(handle, params) < 0 ||
		snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0 ||
		snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE) < 0 ||
		snd_pcm_hw_params_set_channels(handle, params, 1) < 0 ||
		snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir) < 0 ||
		snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir) < 0 ||
		snd_pcm_hw_params(handle, params) < 0) {
		sound_disable();
		if (handle) snd_pcm_close(handle);
		snd_lib_error_set_local(old_handler);
		pthread_mutex_unlock(&sound_mutex);
		return NULL;
	}
	pthread_mutex_unlock(&sound_mutex);

	const uint64_t num_samples = (uint64_t)duration_ms * rate / 1000;
	short buffer[frames];
	int failed = 0;
	for (uint64_t i = 0; i < num_samples && !failed; i += frames) {
		const snd_pcm_uframes_t count = num_samples - i < frames ?
			(snd_pcm_uframes_t)(num_samples - i) : frames;
		for (snd_pcm_uframes_t j = 0; j < count; j++) {
			buffer[j] = 32000 * sin(2.0 * M_PI * frequency * (i + j) / rate);
		}
		for (snd_pcm_uframes_t written = 0; written < count;) {
			snd_pcm_sframes_t result = snd_pcm_writei(handle, buffer + written, count - written);
			if (result < 0 && snd_pcm_recover(handle, (int)result, 1) == 0)
				continue;
			if (result <= 0) {
				pthread_mutex_lock(&sound_mutex);
				sound_disable();
				pthread_mutex_unlock(&sound_mutex);
				failed = 1;
				break;
			}
			written += (snd_pcm_uframes_t)result;
		}
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	snd_lib_error_set_local(old_handler);
	return NULL;
}

void sound(uint32_t frequency, uint32_t duration_ms) {
	pthread_mutex_lock(&sound_mutex);
	const int unavailable = sound_unavailable;
	pthread_mutex_unlock(&sound_mutex);
	if (unavailable) return;
	LoresSoundArgs* args = (LoresSoundArgs*)malloc(sizeof(*args));
	if (!args) return;
	args->frequency = frequency;
	args->duration_ms = duration_ms;
	pthread_t thread;
	if (pthread_create(&thread, NULL, sound_thread, args) == 0)
		pthread_detach(thread);
	else
		free(args);
}
#endif // !LORES_NO_AUDIO
#endif // platform

#ifdef LORES_NO_AUDIO
void sound(uint32_t frequency, uint32_t duration_ms) {
	(void)frequency;
	(void)duration_ms;
}
#endif

// Fall back to 80x24 when neither output nor input reports a usable size.
void scrsize(int* width, int* height) {
	int w = 80, h = 24;
#if defined __WIN32__ || defined WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}
#else
	struct winsize ws;
	if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row) ||
		(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row)) {
		w = ws.ws_col;
		h = ws.ws_row;
	}
#endif
	if (width) *width = w;
	if (height) *height = h;
}

uint64_t timestamp() {
#if defined __WIN32__ || defined WIN32
	return GetTickCount();
#else
	struct timespec ts;
	unsigned theTick = 0U;
	clock_gettime( CLOCK_REALTIME, &ts );
	return (uint64_t)(ts.tv_sec) * 1000 + (uint64_t)(ts.tv_nsec) / 1000000;
#endif
}


//------------------------------------------------------------------
BlockBuf* BlockBufCreate(uint16_t w, uint16_t h) {
	if (h == UINT16_MAX) return NULL; // Rounding must fit in the stored height.
	if(h%2)
		++h;
	const size_t bufsz = sizeof(BlockBuf) + (size_t)w*h;
	BlockBuf* bb = (BlockBuf*)calloc(1, bufsz);
	if (!bb) return NULL;
	bb->w = w;
	bb->h = h;
	return bb;
}

void BlockBufSet(BlockBuf* bb, uint16_t x, uint16_t y, uint8_t color) {
	if(x>=0 && y>=0 && x<bb->w && y<bb->h)
		bb->buf[bb->w*y+x] = color;
}

void BlockBufBlt(BlockBuf* bb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t pattern[]) {
	for(uint16_t j=0; j<h; ++j)
		for(uint16_t i=0; i<w; ++i, ++pattern)
			if(*pattern!=0xff)
				BlockBufSet(bb,x+i, y+j, *pattern);
}

void BlockBufFill(BlockBuf* bb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
	for(uint16_t j=0; j<h; ++j)
		for(uint16_t i=0; i<w; ++i)
			BlockBufSet(bb,x+i, y+j, color);
}


void BlockBufOut(const BlockBuf* bb, uint16_t x, uint16_t y) {
	const uint16_t w = bb->w, stride = w;
	char buf[w*24];
	const uint8_t* pattern = bb->buf;
	for(uint16_t j=0; j<bb->h; j+=2, ++y, pattern += 2*stride) {
		char* p = buf;

		uint8_t prev0, prev1;
		for(uint16_t i=0; i<w; ++i) {
			const uint8_t color0 = pattern[i], color1 = pattern[stride+i];
			if(!i || color0!=prev0 || color1!=prev1) { // set fg/bg color
				p += sprintf(p, "\x1b[38;5;%hu;48;5;%hum", color0, color1);
			}
			if(color0==color1)
				*p++=' ';
			else {
				*p++='\xE2'; *p++='\x96'; *p++='\x80';
			}
			prev0 = color0;
			prev1 = color1;
		}
		printf("\x1b[%hu;%huH%.*s", y,x, (int)(p-buf), buf);
	}
	fputs("\x1b[m", stdout);
	fflush(stdout);
}

static void swapByte(uint8_t buf[], size_t i, size_t j) {
	const uint8_t t = buf[i];
	buf[i] = buf[j];
	buf[j] = t;
}

uint8_t* BlockBufDecode(const char data[], uint16_t w, uint16_t h, const char* colorMap, int flags) {
	if ((flags & FLIP_D) && w != h) return NULL;
	const size_t len = (size_t)w*h;
	uint8_t* buf = (uint8_t*)malloc(len);
	if (!buf) return NULL;
	for(size_t i=0; i<len; ++i) {
		buf[i] = 0xff; // default transparent
		for(size_t j=0, colorMapLen = colorMap[0]; j<colorMapLen; ++j)
			if(colorMap[1+j*2] == data[i]) {
				buf[i] = colorMap[2+j*2];
				break;
			}
	}
	if(flags & FLIP_X)
		for(uint16_t y=0; y<h; ++y)
			for(uint16_t x=0; x<w/2; ++x)
				swapByte(buf, (size_t)y*w + x, (size_t)y*w + w-1-x);
	if(flags & FLIP_Y)
		for(uint16_t y=0; y<h/2; ++y)
			for(uint16_t x=0; x<w; ++x)
				swapByte(buf, (size_t)y*w + x, (size_t)(h-1-y)*w + x);
	if(flags & FLIP_D)
		for(uint16_t y=0; y<h; ++y)
			for(uint16_t x=0; x<y; ++x)
				swapByte(buf, (size_t)y*w + x, (size_t)x*w + y);
	return buf;
}

#endif // LORES_H_IMPLEMENTATION
