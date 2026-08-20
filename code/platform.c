
#pragma once

local void* ReserveMemory(usize Size);
local void CommitMemory(void* Memory, usize Size);

local void* MapExecutableMemory(void* Data, usize Size);
local void UnmapExecutableMemory(void* Memory, usize Size);

local usize WriteStdOut(void* Data, usize Size);
local usize WriteStdErr(void* Data, usize Size);

local void Exit(u8 ExitCode);

#if PlatformWindows
#error Not implemented yet
#elif PlatformLinux
#include "platform_linux.c"
#else
#error No implementation available for platform
#endif

