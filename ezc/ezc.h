#ifndef EZC_H
#define EZC_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- printfc (colored console text) ---------------- */

typedef enum PfcColor {
    GRAY = 0,
    GREY = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7
} PfcColor;

int printfc(PfcColor fg_color, PfcColor bg_color, const char *format, ...);
int printfc_fg(PfcColor fg_color, const char *format, ...);
int printfc_bg(PfcColor bg_color, const char *format, ...);

/* ---------------- logic gate helpers ---------------- */

typedef enum {
    ON = 1,
    OFF = 0
} WIRE;

#define AND(i,j) if((i) == ON && (j) == ON)
#define OR(i,j) if((i) == ON || (j) == ON)
#define NAND(i,j) if((i) == OFF && (j) == OFF)
#define NOR(i,j) if((i) == OFF || (j) == OFF)
#define SPLIT(i,j) WIRE (j) = (i)

/* ---------------- controller ---------------- */

#define Sright 32767
#define Sleft -32767
#define Sup -32767
#define Sdown 32767
#define Scenter 0

typedef struct {
    int device; // controller file

    // buttons
    int A;
    int B;
    int X;
    int Y;

    int L1;
    int R1;
    int L2;
    int R2;

    int START;
    int SELECT;

    int UP;
    int DOWN;
    int LEFT;
    int RIGHT;

    // sticks
    float leftStickX;
    float leftStickY;

    float rightStickX;
    float rightStickY;
} Controller;

int controller_setup(Controller *controller);
void controller_update(Controller *controller);

/* ---------------- terminal input (raw char mode) ---------------- */
/* Renamed from KEY_* to CH_* to avoid colliding with the Key enum below */

#define CH_BACKSPACE 127
#define CH_TAB '\t'
#define CH_SPACE ' '
#define CH_ESC 27
#define CH_ENTER '\n'

void startInput(void);
int keypressed_char(char key); // renamed from keypressed(char) - collided with keypressed(Key)
void stopInput(void);
void getkey(char *var);
char getkeychar(void);

/* ---------------- networking ---------------- */

int nw_server(int port);
int nw_connect(int port);
int nw_recv(int a, char b[100], int c, int d);
void nw_send(int a, char b[100], int len, int flags);
int nw_accept(int server);

/* ---------------- parent/child vectors ---------------- */

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    Vector2 parent;
    Vector2 child;
} Parentobj;

typedef struct {
    Vector2 parent;
    Vector2 child1;
    Vector2 child2;
} Dparentobj;

Parentobj parent(Vector2 parent, Vector2 child);
void parentupdate(Parentobj *par); // now takes a pointer, so updates actually persist
Dparentobj Dparent(Vector2 parent, Vector2 child1, Vector2 child2);
void Dparentupdate(Dparentobj *par); // same fix

/* ---------------- ez screen (terminal block-pixel canvas) ---------------- */

extern int H;
extern int W;

#define RIGHT_DOWN 0
#define RIGHT_UP   1
#define LEFT_DOWN  2
#define LEFT_UP    3

int **ez_startScreen(int height, int width);
void ez_printscreen(int **Screen);
void ez_drawrect(int color, int x, int y, int l, int h, int **screen);
void ez_drawcircle(int color, int x, int y, int r, int **screen);
void ez_background(int color, int **screen); // fixed: was "int screen"
void ez_stopscreen(void);
void ez_drawDiagonalRect(int color, int x, int y, int length, int height, int direction, int **screen);

/* ---------------- eztext (line-based text file editing) ---------------- */

char *readline(int targetLine, const char *filename);
void writeline(const char *text, int targetLine, const char *filename);
void addline(int line, char *file, char *text);

/* ---------------- screenpaint (real GUI window, pixel canvas) ---------------- */

typedef struct {
    int a; // red
    int b; // green
    int c; // blue
} v3;

typedef struct {
    v3 color;
    int x;
    int y;
} Brush;

typedef enum {
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SPACE, KEY_ENTER, KEY_ESC,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT
} Key;

int keypressed(Key key);
void opencanvas(int width, int height, const char *title);
void get_screen_size(int *width, int *height);
void putpixel(int x, int y, int r, int g, int b); // fixed spelling, was "putpixle"
int is_same_or_adjacent(int x1, int y1, int x2, int y2);
void brushdown(Brush brush); // was declared twice, now once
void clearscreen(v3 color);
void putimage(Brush brush, v3 *image, int w, int h);
v3 *load_image(const char *filename, int *w, int *h);
void get_mouse_state(int *x, int *y, int *down);
int process_events(void);

#ifdef __cplusplus
}
#endif

#endif // EZC_H
