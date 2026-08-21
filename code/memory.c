
// ==========================================================================================
// NOTE(vak): A collection of memory management structures and functions.
// ==========================================================================================

#pragma once

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
    void* Base;
    usize Used;
    usize Commited;
    usize Reserved;
} arena;

local arena Arenas[32] = {0};
local u32 ArenaCount = 0;

local arena* GetArena(arena_id ArenaID)
{
    AlwaysAssert(ArenaID.U32[0] > 0);
    AlwaysAssert(ArenaID.U32[0] < ArenaCount + 1);

    arena* Arena = Arenas + (ArenaID.U32[0] - 1);
    return (Arena);
}

local arena_id CreateArena(usize MinCommited, usize MinReserved)
{
    AlwaysAssert(ArenaCount < ArrayCount(Arenas));

    arena_id ArenaID = {.U32[0] = 1 + ArenaCount};
    ArenaCount++;

    arena* Arena = GetArena(ArenaID);

    Arena->Reserved = AlignUp(MinReserved, ArenaGranuleSize);
    Arena->Commited = AlignUp(MinCommited, ArenaGranuleSize);

    AlwaysAssert(Arena->Reserved >  0);
    AlwaysAssert(Arena->Reserved >= Arena->Commited);

    Arena->Base = ReserveMemory(Arena->Reserved);

    if (Arena->Commited)
        CommitMemory(Arena->Base, Arena->Commited);

    return (ArenaID);
}

local void ResetArena(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    Arena->Used = 0;
}

local void* GetArenaBase(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    void* Result = Arena->Base;
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
    arena* Arena = GetArena(ArenaID);
    void* Result = (u8*)Arena->Base + Arena->Used;
    return (Result);
}

local void* PushArenaSize(arena_id ArenaID, usize Size)
{
    arena* Arena = GetArena(ArenaID);

    if (Arena->Used + Size > Arena->Commited)
    {
        usize ExpandSize = (Arena->Used + Size) - Arena->Commited;
        usize CommitSize = AlignUp(ExpandSize, ArenaGranuleSize);
        void* CommitAt   = (u8*)Arena->Base + Arena->Commited;

        if (Arena->Commited + CommitSize > Arena->Reserved)
            Exit(1);

        CommitMemory(CommitAt, CommitSize);

        Arena->Commited += CommitSize;
    }

    void* Result = (u8*)Arena->Base + Arena->Used;
    Arena->Used += Size;

    return (Result);
}

