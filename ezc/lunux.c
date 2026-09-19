#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>

static Display *g_display;
static Window g_window;
static GC g_gc; // fixed: was used in putpixel but never declared/created

void opencanvas(int width, int height, const char* title)
{
    g_display = XOpenDisplay(NULL);
    int screen = DefaultScreen(g_display);

    g_window = XCreateSimpleWindow(
        g_display, RootWindow(g_display, screen),
        10, 10, width, height, 1,
        BlackPixel(g_display, screen),
        WhitePixel(g_display, screen)
    );

    g_gc = XCreateGC(g_display, g_window, 0, NULL); // fixed: now actually created

    XStoreName(g_display, g_window, title);
    XSelectInput(g_display, g_window, ExposureMask | KeyPressMask);
    XMapWindow(g_display, g_window);
}

void get_screen_size(int* width, int* height)
{
    Display* display = XOpenDisplay(NULL);
    int screen = DefaultScreen(display);
    *width = XDisplayWidth(display, screen);
    *height = XDisplayHeight(display, screen);
    XCloseDisplay(display);
}

void putpixel(int x, int y, int r, int g, int b) // fixed: was "putpixle"
{
    unsigned long color = (r << 16) | (g << 8) | b;
    XSetForeground(g_display, g_gc, color);
    XDrawPoint(g_display, g_window, g_gc, x, y);
    XFlush(g_display);
}

int is_same_or_adjacent(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    return (dx <= 1 && dy <= 1);
}

void brushdown(Brush brush)
{
    int a, b;
    get_screen_size(&a, &b);
    // fixed: loops were backwards (i >= a), never ran at all
    for (int i = 0; i < a; i++) {
        for (int j = 0; j < b; j++) {
            if (is_same_or_adjacent(i, j, brush.x, brush.y)) {
                putpixel(i, j, brush.color.a, brush.color.b, brush.color.c);
            }
        }
    }
}

void clearscreen(v3 color)
{
    int a, b;
    get_screen_size(&a, &b);
    // fixed: loops were backwards, and there was a broken nested/unclosed call
    for (int i = 0; i < a; i++) {
        for (int j = 0; j < b; j++) {
            putpixel(i, j, color.a, color.b, color.c);
        }
    }
}

void putimage(Brush brush, v3* image, int w, int h)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            v3 p = image[y * w + x];
            putpixel(brush.x + x, brush.y + y, p.a, p.b, p.c);
        }
    }
}

int process_events(void)
{
    while (XPending(g_display)) {
        XEvent event;
        XNextEvent(g_display, &event);
        if (event.type == KeyPress) {
            KeySym key = XLookupKeysym(&event.xkey, 0);
            if (key == XK_Escape) return 0;
        }
    }
    return 1;
}

void get_mouse_state(int* x, int* y, int* down)
{
    Window root_return, child_return;
    int root_x, root_y;
    unsigned int mask_return;

    XQueryPointer(g_display, g_window, &root_return, &child_return,
                  &root_x, &root_y, x, y, &mask_return);

    *down = (mask_return & Button1Mask) ? 1 : 0;
}

int keypressed(Key key)
{
    char keys[32];
    XQueryKeymap(g_display, keys);

    KeySym keysym;

    if (key >= KEY_A && key <= KEY_Z) {
        keysym = XK_a + (key - KEY_A);
    } else if (key >= KEY_0 && key <= KEY_9) {
        keysym = XK_0 + (key - KEY_0);
    } else {
        switch (key) {
            case KEY_SPACE: keysym = XK_space;  break;
            case KEY_ENTER: keysym = XK_Return; break;
            case KEY_ESC:   keysym = XK_Escape; break;
            case KEY_UP:    keysym = XK_Up;     break;
            case KEY_DOWN:  keysym = XK_Down;   break;
            case KEY_LEFT:  keysym = XK_Left;   break;
            case KEY_RIGHT: keysym = XK_Right;  break;
            default: return 0;
        }
    }

    KeyCode keycode = XKeysymToKeycode(g_display, keysym);
    return (keys[keycode / 8] & (1 << (keycode % 8))) != 0;
}
