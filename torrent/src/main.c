#include <windows.h>

#include "tracker.h"
#include "ui.h"

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmdline, int show)
{
    (void)prev;
    (void)cmdline;
    tr_global_init();
    int rc = ui_run(inst, show);
    tr_global_cleanup();
    return rc;
}
