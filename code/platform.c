
// ==========================================================================================
// NOTE(vak): Platform-related definitions
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef struct
{
    u64 U64[2];
    u32 U32[4];
    u16 U16[8];
    u8  U8 [16];
} memory_id;

#define NilMemoryID (memory_id){0}
#define IsNilMemoryID(MemoryID) (((MemoryID).U64[0] == 0) || ((MemoryID.U64[1]) == 0))

local memory_id ReserveMemory   (usize Size);
local b32       ReleaseMemory   (memory_id MemoryID);
local void*     GetMemoryBase   (memory_id MemoryID);
local usize     GetMemorySize   (memory_id MemoryID);
local b32       CommitMemory    (memory_id MemoryID, usize From, usize Size);
local b32       DecommitMemory  (memory_id MemoryID, usize From, usize Size);

local usize     WriteStdOut     (void* Data, usize Size, ...);
local usize     WriteStdErr     (void* Data, usize Size, ...);

local void      Exit            (u8 ExitCode);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

#if PlatformWindows
#error Not implemented yet
#elif PlatformLinux
#include "platform_linux.c"
#else
#error No implementation available for platform
#endif

