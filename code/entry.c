
#include "shared.c"
#include "platform.c"
#include "print.c"
#include "lexer.c"
#include "parser.c"
#include "intermediate.c"
#include "generator.c"
#include "main.c"

#if PlatformLinux
__attribute__((force_align_arg_pointer))
#endif

void EntryPoint(void)
{
    Main();
    Exit(0);
}

