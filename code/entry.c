
// ==========================================================================================
// NOTE(vak): Standard entry point and main compilation file
// ==========================================================================================

#include "shared.c"
#include "platform.c"
#include "memory.c"
#include "print.c"
#include "main.c"

#if PlatformLinux
    // NOTE(vak): This attribute forces the stack pointer to be aligned to some standard alignment
    // (usually a 16-byte boundary). This prevents issues on Linux where the stack may initially be
    // aligned to an 8-byte boundary, and that can cause issues with SIMD instructions that expects
    // an address that is aligned to a 16-byte boundary.

    // TODO(vak): The entry point on Linux should be written with assembly to handle command line
    // arguments, and also to ensure safe stack pointer alignment.

    __attribute__((force_align_arg_pointer))
#endif

void EntryPoint(void)
{
    Main();
    Exit(0);
}

