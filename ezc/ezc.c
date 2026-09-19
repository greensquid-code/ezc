#include "ezc.h"
#include <string.h>

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* ================= printfc ================= */

static bool is_color_valid(PfcColor color)
{
    return color >= 0 && color <= 7;
}

static const char *get_fg_color_code(PfcColor color) {
    const char *colors[] = {
        "\033[30m", "\033[34m", "\033[32m", "\033[36m",
        "\033[31m", "\033[35m", "\033[33m", "\033[37m"
    };
    return is_color_valid(color) ? colors[color] : NULL;
}

static const char *get_bg_color_code(PfcColor color) {
    const char *colors[] = {
        "\033[40m", "\033[44m", "\033[42m", "\033[46m",
        "\033[41m", "\033[45m", "\033[43m", "\033[47m"
    };
    return is_color_valid(color) ? colors[color] : NULL;
}

#ifdef _WIN32

int printfc(PfcColor fg_color, PfcColor bg_color, const char *format, ...) {
    if (!is_color_valid(fg_color) || !is_color_valid(bg_color)) { errno = EINVAL; return -1; }
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi_info;
    if (!GetConsoleScreenBufferInfo(h_console, &csbi_info)) { errno = EINVAL; return -1; }
    va_list args;
    va_start(args, format);
    SetConsoleTextAttribute(h_console, fg_color | (bg_color << 4));
    int chars_written = vprintf(format, args);
    SetConsoleTextAttribute(h_console, csbi_info.wAttributes);
    va_end(args);
    return chars_written;
}

int printfc_fg(PfcColor fg_color, const char *format, ...) {
    if (!is_color_valid(fg_color)) { errno = EINVAL; return -1; }
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi_info;
    if (!GetConsoleScreenBufferInfo(h_console, &csbi_info)) { errno = EINVAL; return -1; }
    va_list args;
    va_start(args, format);
    SetConsoleTextAttribute(h_console, fg_color);
    int chars_written = vprintf(format, args);
    SetConsoleTextAttribute(h_console, csbi_info.wAttributes);
    va_end(args);
    return chars_written;
}

int printfc_bg(PfcColor bg_color, const char *format, ...) {
    if (!is_color_valid(bg_color)) { errno = EINVAL; return -1; }
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi_info;
    if (!GetConsoleScreenBufferInfo(h_console, &csbi_info)) { errno = EINVAL; return -1; }
    va_list args;
    va_start(args, format);
    SetConsoleTextAttribute(h_console, (csbi_info.wAttributes & 0x0F) | (bg_color << 4));
    int chars_written = vprintf(format, args);
    SetConsoleTextAttribute(h_console, csbi_info.wAttributes);
    va_end(args);
    return chars_written;
}

#else // Linux / Unix-like

int printfc(PfcColor fg_color, PfcColor bg_color, const char *format, ...) {
    if (!is_color_valid(fg_color) || !is_color_valid(bg_color)) { errno = EINVAL; return -1; }
    va_list args;
    va_start(args, format);
    printf("%s%s", get_fg_color_code(fg_color), get_bg_color_code(bg_color));
    int chars_written = vprintf(format, args);
    printf("\033[0m");
    va_end(args);
    return chars_written;
}

int printfc_fg(PfcColor fg_color, const char *format, ...) {
    if (!is_color_valid(fg_color)) { errno = EINVAL; return -1; }
    va_list args;
    va_start(args, format);
    printf("%s", get_fg_color_code(fg_color));
    int chars_written = vprintf(format, args);
    printf("\033[0m");
    va_end(args);
    return chars_written;
}

int printfc_bg(PfcColor bg_color, const char *format, ...) {
    if (!is_color_valid(bg_color)) { errno = EINVAL; return -1; }
    va_list args;
    va_start(args, format);
    printf("%s", get_bg_color_code(bg_color));
    int chars_written = vprintf(format, args);
    printf("\033[0m");
    va_end(args);
    return chars_written;
}

#endif

/* ================= controller (Linux joystick API) ================= */

#if defined(__linux__)

static int controller_fd = -1;

int controller_setup(Controller *controller)
{
    controller_fd = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
    if (controller_fd == -1) {
        printf("No controller found!\n");
        return 0;
    }

    controller->device = controller_fd;
    controller->A = controller->B = controller->X = controller->Y = 0;
    controller->L1 = controller->R1 = controller->L2 = controller->R2 = 0;
    controller->START = controller->SELECT = 0;
    controller->UP = controller->DOWN = controller->LEFT = controller->RIGHT = 0;
    controller->leftStickX = controller->leftStickY = 0;
    controller->rightStickX = controller->rightStickY = 0;

    printf("Controller connected!\n");
    return 1;
}

void controller_update(Controller *controller)
{
    struct js_event event;

    while (read(controller_fd, &event, sizeof(event)) > 0)
    {
        event.type &= ~JS_EVENT_INIT;

        if (event.type == JS_EVENT_BUTTON)
        {
            int pressed = event.value;
            switch (event.number)
            {
                case 0: controller->A = pressed; break;
                case 1: controller->B = pressed; break;
                case 2: controller->X = pressed; break;
                case 3: controller->Y = pressed; break;
                case 4: controller->L1 = pressed; break;
                case 5: controller->R1 = pressed; break;
                case 6: controller->SELECT = pressed; break;
                case 7: controller->START = pressed; break;
            }
        }

        if (event.type == JS_EVENT_AXIS)
        {
            // fixed: no more undeclared lx/lY/rX/rY, no unreachable code after break,
            // conversion done directly per-axis
            switch (event.number)
            {
                case 0: controller->leftStickX = event.value / 32767.0f; break;
                case 1: controller->leftStickY = event.value / 32767.0f; break;
                case 2: controller->rightStickX = event.value / 32767.0f; break;
                case 3: controller->rightStickY = event.value / 32767.0f; break;
            }
        }
    }
}

/* ================= terminal raw input ================= */

static struct termios oldt, newt;

void startInput(void)
{
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

int keypressed_char(char key)
{
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1 && c == key) return 0; // pressed (matches your original rule)
    return 1; // not pressed
}

void stopInput(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

char getkeychar(void)
{
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return 0;
}

void getkey(char *var)
{
    var[0] = getkeychar();
    var[1] = '\0';
}

/* ================= networking ================= */

int nw_server(int port)
{
    int sockfd;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket failed"); return -1; }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int nw_connect(int port)
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket failed"); return -1; }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        close(sock);
        return -1;
    }

    return sock;
}

void nw_send(int a, char b[100], int len, int flags)
{
    // fixed: sends the actual message length instead of always 100 bytes
    send(a, b, len, flags);
}

int nw_recv(int a, char b[100], int c, int d)
{
    int e = recv(a, b, c, d);
    if (e > 0) b[e] = '\0'; // fixed: only null-terminate on success, avoids b[-1] on error
    return e;
}

int nw_accept(int server)
{
    return accept(server, NULL, NULL); // fixed: was missing return
}

#endif // __linux__

/* ================= parent/child vectors ================= */

Parentobj parent(Vector2 parentPos, Vector2 child)
{
    Parentobj par;
    par.parent = parentPos;
    par.child.x = parentPos.x + child.x; // fixed: was parent.x + child.y
    par.child.y = parentPos.y + child.y; // fixed: was parent.x + child.y
    return par;
}

void parentupdate(Parentobj *par)
{
    // fixed: takes a pointer so the update actually persists, fixed x/y mixup
    par->child.x = par->parent.x + par->child.x;
    par->child.y = par->parent.y + par->child.y;
}

Dparentobj Dparent(Vector2 parentPos, Vector2 child1, Vector2 child2)
{
    Dparentobj par; // fixed: was declared as Parentobj (wrong type, missing child2)
    par.parent = parentPos;
    par.child1.x = parentPos.x + child1.x;
    par.child1.y = parentPos.y + child1.y;
    par.child2.x = parentPos.x + child2.x;
    par.child2.y = parentPos.y + child2.y;
    return par; // fixed: was missing return entirely
}

void Dparentupdate(Dparentobj *par)
{
    par->child1.x = par->parent.x + par->child1.x;
    par->child1.y = par->parent.y + par->child1.y;
    par->child2.x = par->parent.x + par->child2.x;
    par->child2.y = par->parent.y + par->child2.y;
}

/* ================= ez screen (terminal block-pixel canvas) ================= */

int H;
int W;

int **ez_startScreen(int height, int width)
{
    H = height;
    W = width;
    int **array = malloc(sizeof(int*) * H);
    for (int i = 0; i < H; i++) {
        array[i] = malloc(sizeof(int) * W);
    }
    return array;
}

void ez_printscreen(int **Screen)
{
    system("clear");
    // fixed: loop bounds were swapped (was i<W outer, j<H inner) - now matches H rows / W cols
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            printfc_fg(Screen[i][j], "%s", "\u2588");
        }
        printf("\n");
    }
}

void ez_drawrect(int color, int x, int y, int l, int h, int **screen)
{
    for (int xt = 0; xt < l; xt++) {
        for (int yt = 0; yt < h; yt++) {
            screen[y + yt][x + xt] = color;
        }
    }
}

void ez_drawcircle(int color, int x, int y, int r, int **screen)
{
    // fixed: real squared-distance check instead of (i-x)+(j-y), which allowed
    // negative terms to cancel out and drew the wrong shape
    for (int i = 0; i < W; i++) {
        for (int j = 0; j < H; j++) {
            if ((i - x) * (i - x) + (j - y) * (j - y) <= r * r) {
                screen[j][i] = color;
            }
        }
    }
}

void ez_background(int color, int **screen)
{
    for (int i = 0; i < W; i++) {
        for (int j = 0; j < H; j++) {
            screen[j][i] = color;
        }
    }
}

void ez_stopscreen(void)
{
    // no-op now - font size is no longer changed via ydotool,
    // kept as a stub in case other code still calls it
}

void ez_drawDiagonalRect(int color, int x, int y, int length, int height, int direction, int **screen)
{
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < height; j++) {
            int dx = 0, dy = 0;
            if (direction == RIGHT_DOWN) { dx = i; dy = i; }
            if (direction == RIGHT_UP)   { dx = i; dy = -i; }
            if (direction == LEFT_DOWN)  { dx = -i; dy = i; }
            if (direction == LEFT_UP)    { dx = -i; dy = -i; }
            screen[y + j + dy][x + dx] = color;
        }
    }
}

/* ================= eztext ================= */

char *readline(int targetLine, const char *filename)
{
    static char line[256];
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    int currentLine = 1;
    while (fgets(line, sizeof(line), file)) {
        if (currentLine == targetLine) {
            fclose(file);
            return line;
        }
        currentLine++;
    }
    fclose(file);
    return NULL;
}

void writeline(const char *text, int targetLine, const char *filename)
{
    char lines[1000][256];
    int count = 0;

    FILE *file = fopen(filename, "r");
    if (file) {
        while (fgets(lines[count], sizeof(lines[count]), file)) {
            count++;
        }
        fclose(file);
    }

    snprintf(lines[targetLine - 1], 256, "%s\n", text);

    file = fopen(filename, "w");
    for (int i = 0; i < count; i++) {
        fputs(lines[i], file);
    }
    fclose(file);
}

void addline(int line, char *file, char *text)
{
    char hi[256] = ""; // fixed: was 62 bytes, too small for a 256-byte line + text
    char *existing = readline(line, file);
    if (existing) strcpy(hi, existing);
    strcat(hi, text);
    writeline(hi, line, file);
}

/* ================= image loading (shared by all platforms) ================= */

v3 *load_image(const char *filename, int *w, int *h)
{
    int channels;
    unsigned char *data = stbi_load(filename, w, h, &channels, 4);
    if (!data) return NULL;

    v3 *pixels = malloc((size_t)(*w) * (*h) * sizeof(v3));
    if (!pixels) {
        stbi_image_free(data);
        return NULL;
    }

    for (int y = 0; y < *h; y++) {
        for (int x = 0; x < *w; x++) {
            int idx = (y * (*w) + x) * 4;
            int i = y * (*w) + x;
            pixels[i].a = data[idx];
            pixels[i].b = data[idx + 1];
            pixels[i].c = data[idx + 2];
        }
    }

    stbi_image_free(data);
    return pixels;
}

/* ================= screenpaint (platform windowing) ================= */

#if defined(_WIN32)
    #include "win.c"
#elif defined(__linux__)
    #include "lunux.c"
#else
    #error "Unsupported platform"
#endif
