
#include "shared.c"
#include "platform.c"
#include "print.c"
#include "main.c"

#if PlatformLinux
__attribute__((force_align_arg_pointer))
#endif

void EntryPoint(void)
{
    Main();
    Exit(0);
}

