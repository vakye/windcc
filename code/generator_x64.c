
// ==========================================================================================
// NOTE(vak): Compiler x64 generator: Emits x64 machine code from a program's parsed syntax
// tree.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
#include "memory.c"
#include "parser.c"

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef struct
{
    void* Base;
    usize Size;
} x64_code;

local void x64_SetupGenerator(void);
local void x64_ShutdownGenerator(void);

local x64_code x64_Generate(node_id RootNodeID);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

#define x64_DefaultCodeArenaCommited (MB(1))
#define x64_DefaultCodeArenaReserved (GB(64))

local arena_id x64_CodeArenaID = NilArenaID; // NOTE(vak): Storage for generated machine code

local void x64_SetupGenerator(void)
{
    x64_CodeArenaID = CreateArena(
        x64_DefaultCodeArenaCommited,
        x64_DefaultCodeArenaReserved
    );
}

local void x64_ShutdownGenerator(void)
{
    DestroyArena(x64_CodeArenaID);
    x64_CodeArenaID = NilArenaID;
}

local void x64_EmitBytes(void* Bytes, usize Size)
{
    void* WriteAt = PushArenaSize(x64_CodeArenaID, Size);

    CopyMemory(WriteAt, Bytes, Size);
}

local void x64_Emit8 (u8  Value) { x64_EmitBytes(&Value, 1); }
local void x64_Emit16(u16 Value) { x64_EmitBytes(&Value, 2); }
local void x64_Emit24(u32 Value) { x64_EmitBytes(&Value, 3); }
local void x64_Emit32(u32 Value) { x64_EmitBytes(&Value, 4); }
local void x64_Emit40(u64 Value) { x64_EmitBytes(&Value, 5); }
local void x64_Emit48(u64 Value) { x64_EmitBytes(&Value, 6); }
local void x64_Emit56(u64 Value) { x64_EmitBytes(&Value, 7); }
local void x64_Emit64(u64 Value) { x64_EmitBytes(&Value, 8); }

local void x64_GenerateNode(node_id NodeID)
{
    switch (GetNodeKind(NodeID))
    {
        default: {} break;

        case NodeKind_Integer:
        {
            integer_node Integer = GetIntegerNode(NodeID);

            // NOTE(vak):
            // 48 b8 Imm64      mov rax, Imm64
            x64_Emit16(0xb848);
            x64_Emit64(Integer.Value);
        } break;
    }
}

local x64_code x64_Generate(node_id RootNodeID)
{
    ResetArena(x64_CodeArenaID);

    // NOTE(vak): Prologue
    {
        // NOTE(vak):
        // 55           push rbp
        // 48 8b ec     mov rbp, rsp
        x64_Emit32(0xec8b4855);
    }

    // NOTE(vak): Body
    {
        x64_GenerateNode(RootNodeID);
    }

    // NOTE(vak): Epilogue
    {
        // NOTE(vak):
        // 48 8b e5     mov rsp, rbp
        // 5d           pop rbp
        // c3           ret
        x64_Emit40(0xc35de58b48);
    }

    x64_code Result =
    {
        .Base = GetArenaBase(x64_CodeArenaID),
        .Size = GetArenaUsed(x64_CodeArenaID),
    };

    return (Result);
}

