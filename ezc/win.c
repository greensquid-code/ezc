#include <windows.h>
#include <ctype.h>
#include <stdlib.h>

static HWND g_hwnd;
static HDC g_hdc; // fixed: was used in putpixel but never declared/created

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void opencanvas(int width, int height, const char* title)
{
    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "CanvasClass";
    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(0, "CanvasClass", title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInst, NULL);

    g_hdc = GetDC(g_hwnd); // fixed: now actually created

    ShowWindow(g_hwnd, SW_SHOW);
}

void get_screen_size(int* width, int* height)
{
    *width = GetSystemMetrics(SM_CXSCREEN);
    *height = GetSystemMetrics(SM_CYSCREEN);
}

void putpixel(int x, int y, int r, int g, int b)
{
    SetPixel(g_hdc, x, y, RGB(r, g, b));
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
                putpixel(i, j, brush.color.a, brush.color.b, brush.color.c); // fixed typo: was putpixle
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
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return 0;
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) return 0;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 1;
}

void get_mouse_state(int* x, int* y, int* down)
{
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(g_hwnd, &pt);
    *x = pt.x;
    *y = pt.y;
    *down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
}

int keypressed(Key key)
{
    int vk;

    if (key >= KEY_A && key <= KEY_Z) {
        vk = 'A' + (key - KEY_A);
    } else if (key >= KEY_0 && key <= KEY_9) {
        vk = '0' + (key - KEY_0);
    } else {
        switch (key) {
            case KEY_SPACE: vk = VK_SPACE;  break;
            case KEY_ENTER: vk = VK_RETURN; break;
            case KEY_ESC:   vk = VK_ESCAPE; break;
            case KEY_UP:    vk = VK_UP;     break;
            case KEY_DOWN:  vk = VK_DOWN;   break;
            case KEY_LEFT:  vk = VK_LEFT;   break;
            case KEY_RIGHT: vk = VK_RIGHT;  break;
            default: return 0;
        }
    }

    SHORT state = GetAsyncKeyState(vk);
    return (state & 0x8000) ? 1 : 0;
}
