
// ==========================================================================================
// NOTE(vak): A collection of memory management structures and functions.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
#include "platform.c"
#include "print.c"

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef union
{
    u32 U32[1];
    u16 U16[2];
    u8  U8 [4];
} arena_id;

#define NilArenaID (arena_id){0}
#define IsNilArenaID(ArenaID) ((ArenaID).U32[0] == 0)

local arena_id      CreateArena                 (usize MinCommited, usize MinReserved);
local void          DestroyArena                (arena_id ArenaID);
local void          ResetArena                  (arena_id ArenaID);
local void*         GetArenaBase                (arena_id ArenaID);
local usize         GetArenaUsed                (arena_id ArenaID);
local void*         GetArenaAllocationPointer   (arena_id ArenaID);
local void*         PushArenaSize               (arena_id ArenaID, usize Size);

#define PushArena(ArenaID, Type)                (Type*)PushArenaSize(ArenaID, sizeof(Type))
#define PushArenaArray(ArenaID, Type, Count)    (Type*)PushArenaSize(ArenaID, sizeof(Type) * (Count))

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

#define ArenaGranuleSize MB(1)

typedef struct
{
    memory_id   MemoryID;
    usize       Commited;
    usize       Used;
} arena;

local arena Arenas[512] = {0};

local arena* GetArena(arena_id ArenaID)
{
    AlwaysAssert(ArenaID.U32[0] > 0);
    AlwaysAssert(ArenaID.U32[0] <= ArrayCount(Arenas));

    arena* Arena = Arenas + (ArenaID.U32[0] - 1);
    return (Arena);
}

local arena_id FindFreeArenaSlot(void)
{
    arena_id ArenaID = NilArenaID;

    for (u32 Index = 0; Index < ArrayCount(Arenas); Index++)
    {
        arena* Arena = Arenas + Index;

        if (IsNilMemoryID(Arena->MemoryID))
        {
            ArenaID.U32[0] = 1 + Index;
            break;
        }
    }

    return (ArenaID);
}

local arena_id CreateArena(usize MinCommited, usize MinReserved)
{
    arena_id ArenaID = FindFreeArenaSlot();
    AlwaysAssert(!IsNilArenaID(ArenaID));

    arena* Arena = GetArena(ArenaID);

    usize Commited = AlignUp(MinCommited, ArenaGranuleSize);
    usize Reserved = AlignUp(MinReserved, ArenaGranuleSize);

    Arena->MemoryID = ReserveMemory(Reserved);
    if (IsNilMemoryID(Arena->MemoryID))
    {
        Println(StdErr, Str("ERROR: failed to reserve memory for arena"));
        Exit(1);
    }

    Arena->Commited = Commited;
    Arena->Used = 0;

    if (Commited)
    {
        if (!CommitMemory(Arena->MemoryID, 0, Arena->Commited))
        {
            Println(StdErr, Str("ERROR: failed to initially commit memory for arena"));
            Exit(1);
        }
    }

    return (ArenaID);
}

local void DestroyArena(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    ReleaseMemory(Arena->MemoryID);
    ZeroType(Arena);
}

local void ResetArena(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    Arena->Used = 0;
}

local void* GetArenaBase(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    void* Result = GetMemoryBase(Arena->MemoryID);
    return (Result);
}

local usize GetArenaUsed(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    usize Result = Arena->Used;
    return (Result);
}

local void* GetArenaAllocationPointer(arena_id ArenaID)
{
    void* Result = (u8*)GetArenaBase(ArenaID) + GetArenaUsed(ArenaID);
    return (Result);
}

local void* PushArenaSize(arena_id ArenaID, usize Size)
{
    arena* Arena = GetArena(ArenaID);

    if (Arena->Used + Size > Arena->Commited)
    {
        usize ExpandSize = (Arena->Used + Size) - Arena->Commited;
        usize CommitSize = AlignUp(ExpandSize, ArenaGranuleSize);
        usize CommitFrom = Arena->Commited;

        if (!CommitMemory(Arena->MemoryID, CommitFrom, CommitSize))
        {
            Println(StdErr, Str("ERROR: failed to commit more memory for arena"));
            Exit(1);
        }

        Arena->Commited += CommitSize;
    }

    void* Result = GetArenaAllocationPointer(ArenaID);
    Arena->Used += Size;

    return (Result);
}

