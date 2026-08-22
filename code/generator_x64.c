
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

typedef enum
{
    x64_RAX = 0,
    x64_RCX,
    x64_RDX,
    x64_RBX,
    x64_RSP,
    x64_RBP,
    x64_RSI,
    x64_RDI,
    x64_R8,
    x64_R9,
    x64_R10,
    x64_R11,
    x64_R12,
    x64_R13,
    x64_R14,
    x64_R15,
} x64_register;

typedef enum
{
    x64_OperandKind_Nil = 0,
    x64_OperandKind_Imm,
    x64_OperandKind_Reg,
} x64_operand_kind;

typedef struct
{
    x64_operand_kind Kind;
    union
    {
        usize   Imm;
        u8      Reg;
    };
} x64_operand;

#define x64_ImmOperand(Integer)  (x64_operand){.Kind = x64_OperandKind_Imm, .Imm = Integer}
#define x64_RegOperand(Register) (x64_operand){.Kind = x64_OperandKind_Reg, .Reg = Register}

#define x64_IsImmOperand(Operand) ((Operand).Kind == x64_OperandKind_Imm)
#define x64_IsRegOperand(Operand) ((Operand).Kind == x64_OperandKind_Reg)

#define x64_IsImm8(Value)   ((Value <= 0x7F)        || (Value >= 0xFFFFFFFFFFFFFF80))
#define x64_IsImm32(Value)  ((Value <= 0x7FFFFFFF)  || (Value >= 0xFFFFFFFF80000000))

local x64_operand x64_LoadImm64(u8 Register, usize Value)
{
    // NOTE(vak):
    // 48 (b8 + Reg) Imm64      mov Reg, Imm64
    x64_Emit16(0xb848 + (Register << 8));
    x64_Emit64(Value);

    x64_operand Result = x64_RegOperand(Register);
    return (Result);
}

local x64_operand x64_GenerateNode(node_id NodeID)
{
    x64_operand Result = {0};

    switch (GetNodeKind(NodeID))
    {
        default: {} break;

        case NodeKind_Integer:
        {
            integer_node Integer = GetIntegerNode(NodeID);

            Result = x64_ImmOperand(Integer.Value);

        } break;

        case NodeKind_Add:
        {
            binary_node Binary = GetBinaryNode(NodeID);

            x64_operand Left  = x64_GenerateNode(Binary.Left);
            x64_operand Right = x64_GenerateNode(Binary.Right);

            if (x64_IsImmOperand(Left) && x64_IsImmOperand(Right))
            {
                Left = x64_LoadImm64(x64_RAX, Left.Imm);
            }

            if (x64_IsImmOperand(Right))
            {
                if (x64_IsImm8(Right.Imm))
                {
                    // NOTE(vak):
                    // 48 83 c0 Imm8    add rax, Imm8
                    x64_Emit24(0xc08348);
                    x64_Emit8((u8)Right.Imm);
                }
                else if (x64_IsImm32(Right.Imm))
                {
                    // NOTE(vak):
                    // 48 81 c0 Imm32   add rax, Imm32
                    x64_Emit24(0xc08148);
                    x64_Emit32((u32)Right.Imm);
                }
                else
                {
                    x64_LoadImm64(x64_RCX, Right.Imm);

                    // NOTE(vak):
                    // 48 03 c1     add rax, rcx
                    x64_Emit24(0xc10348);
                }
            }
            else
            {
                Println(StdErr, Str("ERROR: not handled yet....asasf"));
                Exit(1);
            }

            Result = Left;
        } break;
    }

    return (Result);
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

