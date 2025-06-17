#include <windows.h>
#include <stdio.h>

void error(const char* helptext) {
    printf(helptext);
    exit(1);
}

int main() {
    const size_t MAX_CHAR_COUNT = 2048;
    
    HWND hwnd_debug = FindWindow("#32770", TEXT("デバッグウィンドウ"));
    if (!hwnd_debug) error("couldn't find debug window");
    
    HWND hwnd_edit = FindWindowEx(hwnd_debug, NULL, NULL, NULL);
    if (!hwnd_edit) error("couldn't find edit control");
    
    HWND hwnd_static = FindWindowEx(hwnd_debug, hwnd_edit, NULL, NULL);
    if (!hwnd_static) error("couldn't find static control");
    
    HWND hwnd_button = FindWindowEx(hwnd_debug, hwnd_static, NULL, NULL);
    if (!hwnd_button) error("couldn't find button control");
    
    TCHAR log_buf[MAX_CHAR_COUNT]; 
    SendMessage(hwnd_edit, WM_GETTEXT, sizeof(log_buf)/sizeof(log_buf[0]), LPARAM(log_buf));
    // LRESULT res = SendMessage(hwnd_button, BM_CLICK, 0, 0);

    printf(log_buf);

    return 0;
}