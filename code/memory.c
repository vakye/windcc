
#pragma once

#define ArenaGranuleSize MB(1)

typedef u32 arena_id;
#define NilArenaID (0)

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
    AlwaysAssert(ArenaID > NilArenaID);
    AlwaysAssert(ArenaID < ArenaCount + 1);

    arena* Arena = Arenas + (ArenaID - 1);
    return (Arena);
}

local arena_id CreateArena(usize MinCommited, usize MinReserved)
{
    AlwaysAssert(ArenaCount < ArrayCount(Arenas));

    arena_id ArenaID = 1 + ArenaCount;
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

local void* GetArenaAllocationPointer(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    void* Result = (u8*)Arena->Base + Arena->Used;
    return (Result);
}

#define PushArena(ArenaID, Type)                (Type*)PushArenaSize(ArenaID, sizeof(Type))
#define PushArenaArray(ArenaID, Type, Count)    (Type*)PushArenaSize(ArenaID, sizeof(Type) * (Count))

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

