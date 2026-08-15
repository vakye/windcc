
#pragma once

#define InvalidLabelOffset (USizeMax)

typedef struct
{
    usize Offset;
} gen_label;

typedef struct gen_rel32 gen_rel32;
struct gen_rel32
{
    gen_label* Target;
    usize Offset;
    gen_rel32* Next;
};

typedef struct gen_symbol gen_symbol;
struct gen_symbol
{
    string Name;
    usize StackOffset;
    gen_symbol* Next;
};

typedef struct
{
    u8* Base;
    usize Size;
    usize Used;
    usize StackSize;

    gen_rel32* FirstRel32;
    gen_rel32* LastRel32;

    gen_symbol* FirstSymbol;
    gen_symbol* LastSymbol;
} gen_buffer;

local void EmitBytes(gen_buffer* Buffer, void* Bytes, usize Size)
{
    if (Buffer->Base == 0)
    {
        // NOTE(vak): No code generation, only retrieve size of code that will be generated
        Buffer->Used += Size;
    }
    else if (Buffer->Used + Size <= Buffer->Size)
    {
        CopyMemory(Buffer->Base + Buffer->Used, Bytes, Size);
        Buffer->Used += Size;
    }
}

local void Emit8 (gen_buffer* Buffer, u8  Value) { EmitBytes(Buffer, &Value, 1); }
local void Emit16(gen_buffer* Buffer, u16 Value) { EmitBytes(Buffer, &Value, 2); }
local void Emit24(gen_buffer* Buffer, u32 Value) { EmitBytes(Buffer, &Value, 3); }
local void Emit32(gen_buffer* Buffer, u32 Value) { EmitBytes(Buffer, &Value, 4); }
local void Emit40(gen_buffer* Buffer, u64 Value) { EmitBytes(Buffer, &Value, 5); }
local void Emit48(gen_buffer* Buffer, u64 Value) { EmitBytes(Buffer, &Value, 6); }
local void Emit56(gen_buffer* Buffer, u64 Value) { EmitBytes(Buffer, &Value, 7); }
local void Emit64(gen_buffer* Buffer, u64 Value) { EmitBytes(Buffer, &Value, 8); }

local gen_label* AllocateLabel(void)
{
    gen_label* Label = Allocate(sizeof(gen_label));
    Label->Offset = InvalidLabelOffset;
    return (Label);
}

local void PlaceLabel(gen_buffer* Gen, gen_label* Label)
{
    Label->Offset = Gen->Used;
}

local void EmitRel32(gen_buffer* Gen, gen_label* Target)
{
    gen_rel32* Rel32 = Allocate(sizeof(gen_rel32));
    ZeroType(Rel32);

    Rel32->Target = Target;
    Rel32->Offset = Gen->Used;

    Emit32(Gen, 0x00000000);

    if (!Gen->FirstRel32)
    {
        Gen->FirstRel32 = Rel32;
        Gen->LastRel32 = Rel32;
    }
    else
    {
        Gen->LastRel32->Next = Rel32;
        Gen->LastRel32 = Rel32;
    }
}

local gen_symbol* AddSymbol(gen_buffer* Gen, string Name)
{
    gen_symbol* Symbol = Allocate(sizeof(gen_symbol));

    Gen->StackSize += 8;

    ZeroType(Symbol);
    Symbol->Name = Name;
    Symbol->StackOffset = Gen->StackSize;

    if (!Gen->FirstSymbol)
    {
        Gen->FirstSymbol = Symbol;
        Gen->LastSymbol = Symbol;
    }
    else
    {
        Gen->LastSymbol->Next = Symbol;
        Gen->LastSymbol = Symbol;
    }

    return (Symbol);
}

local gen_symbol* LookupSymbol(gen_buffer* Gen, string Name)
{
    gen_symbol* Result = 0;

    for (
        gen_symbol* Symbol = Gen->FirstSymbol;
        Symbol;
        Symbol = Symbol->Next
    )
    {
        if (StringEqual(Symbol->Name, Name))
        {
            Result = Symbol;
            break;
        }
    }

    return (Result);
}

local void GenerateAddress(gen_buffer* Gen, node* Node)
{
    if (!Node)
    {
        Println(Str("ERROR: not a value associated with a memory address"));
        Exit(1);
    }

    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: not a value associated with a memory address"));
            Exit(1);
        } break;

        case NodeKind_Identifier:
        {
            gen_symbol* Symbol = LookupSymbol(Gen, Node->Identifier);
            if (!Symbol)
            {
                Symbol = AddSymbol(Gen, Node->Identifier);
            }

            if (Symbol->StackOffset > S32Max)
            {
                Println(Str("ERROR: variable stack offset exceeds 32-bit signed integer max"));
                Exit(1);
            }

            s32 Displacement = -(s32)Symbol->StackOffset;

            // NOTE(vak):
            // 48 8d 85 (Disp32) lea rax, [rbp - Symbol->StackOffset]
            Emit24(Gen, 0x858d48);
            Emit32(Gen, Displacement);
        } break;
    }
}

local void GenerateNode(gen_buffer* Gen, node* Node)
{
    if (!Node)
        return;

    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: unimplemented node in GenerateNode"));
            Exit(1);
        } break;

        case NodeKind_Integer:
        {
            // NOTE(vak):
            // 48 b8 (Imm64)    mov rax, Imm64
            Emit16(Gen, 0xb848);
            Emit64(Gen, Node->Integer);
        } break;

        case NodeKind_Identifier:
        {
            GenerateAddress(Gen, Node);

            // NOTE(vak):
            // 48 8b 00         mov rax, [rax]
            Emit24(Gen, 0x008b48);
        } break;

        case NodeKind_Negate:
        {
            GenerateNode(Gen, Node->Left);

            // NOTE(vak):
            // 48 f7 d8         neg rax
            Emit24(Gen, 0xd8f748);
        } break;

        case NodeKind_BitwiseNot:
        {
            GenerateNode(Gen, Node->Left);

            // NOTE(vak):
            // 48 f7 d0         not rax
            Emit24(Gen, 0xd0f748);
        } break;

        case NodeKind_LogicalNot:
        {
            GenerateNode(Gen, Node->Left);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 94 c0     setz al
            // 48 0f b6 c0  movzx rax, al
            Emit64(Gen, 0x0f48c0940fc08548);
            Emit16(Gen, 0xc0b6);
        } break;

        case NodeKind_Add:
        case NodeKind_Sub:
        case NodeKind_Mul:
        case NodeKind_Div:
        case NodeKind_Mod:
        case NodeKind_ShiftLeft:
        case NodeKind_ShiftRight:
        case NodeKind_BitwiseAnd:
        case NodeKind_BitwiseXor:
        case NodeKind_BitwiseOr:
        {
            // NOTE(vak): C doesn't seem to enforce a specific order of evaluation
            // so we can evaluate in whichever order we want as long as the results
            // are consistent.

            GenerateNode(Gen, Node->Right);
            Emit8(Gen, 0x50); // NOTE(vak): 50 push rax
            GenerateNode(Gen, Node->Left);
            Emit8(Gen, 0x59); // NOTE(vak): 59 pop rcx

            switch (Node->Kind)
            {
                // NOTE(vak):
                // 48 03 c1         add rax, rcx
                case NodeKind_Add: Emit24(Gen, 0xc10348); break;

                // NOTE(vak):
                // 48 2b c1         sub rax, rcx
                case NodeKind_Sub: Emit24(Gen, 0xc12b48); break;

                // NOTE(vak):
                // 48 0f af c1      imul rax, rcx
                case NodeKind_Mul: Emit32(Gen, 0xc1af0f48); break;

                // NOTE(vak):
                // 48 99            cqo
                // 48 f7 f9         idiv rcx
                case NodeKind_Div: Emit40(Gen, 0xf9f7489948); break;

                // NOTE(vak):
                // 48 99            cqo
                // 48 f7 f9         idiv rcx
                // 48 8b c2         mov rax, rdx
                case NodeKind_Mod: Emit64(Gen, 0xc28b48f9f7489948); break;

                // NOTE(vak):
                // 48 d3 e0         sal rax, cl
                case NodeKind_ShiftLeft: Emit24(Gen, 0xe0d348); break;

                // NOTE(vak):
                // 48 d3 f8         sar rax, cl
                case NodeKind_ShiftRight: Emit24(Gen, 0xf8d348); break;

                // NOTE(vak):
                // 48 23 c1         and rax, rcx
                case NodeKind_BitwiseAnd: Emit24(Gen, 0xc12348); break;

                // NOTE(vak):
                // 48 33 c1         xor rax, rcx
                case NodeKind_BitwiseXor: Emit24(Gen, 0xc13348); break;

                // NOTE(vak):
                // 48 0b c1         or rax, rcx
                case NodeKind_BitwiseOr: Emit24(Gen, 0xc10b48); break;
            }
        } break;

        case NodeKind_Equal:
        case NodeKind_NotEqual:
        case NodeKind_Less:
        case NodeKind_Greater:
        case NodeKind_LessEqual:
        case NodeKind_GreaterEqual:
        {
            GenerateNode(Gen, Node->Right);
            Emit8(Gen, 0x50); // NOTE(vak): 50 push rax
            GenerateNode(Gen, Node->Left);
            Emit8(Gen, 0x59); // NOTE(vak): 59 pop rcx

            // NOTE(vak):
            // 48 3b c1     cmp rax, rcx
            Emit24(Gen, 0xc13b48);

            switch (Node->Kind)
            {
                case NodeKind_Equal:            Emit24(Gen, 0xc0940f); break; // NOTE(vak): 0f 94 c0 sete al
                case NodeKind_NotEqual:         Emit24(Gen, 0xc0950f); break; // NOTE(vak): 0f 95 c0 setne al
                case NodeKind_Less:             Emit24(Gen, 0xc09c0f); break; // NOTE(vak): 0f 9c c0 setl al
                case NodeKind_Greater:          Emit24(Gen, 0xc09f0f); break; // NOTE(vak): 0f 9f c0 setg al
                case NodeKind_LessEqual:        Emit24(Gen, 0xc09e0f); break; // NOTE(vak): 0f 9e c0 setle al
                case NodeKind_GreaterEqual:     Emit24(Gen, 0xc09d0f); break; // NOTE(vak): 0f 9d c0 setge al
            }

            // NOTE(vak):
            // 48 0f b6 c0  movzx rax, al
            Emit32(Gen, 0xc0b60f48);
        } break;

        case NodeKind_LogicalAnd:
        {
            gen_label* SkipRight = AllocateLabel();

            GenerateNode(Gen, Node->Left);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 84 Rel32  jz SkipRight
            Emit40(Gen, 0x840fc08548);
            EmitRel32(Gen, SkipRight);

            GenerateNode(Gen, Node->Right);

            PlaceLabel(Gen, SkipRight);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 95 c0     setnz al
            // 48 0f b6 c0  movzx rax, al
            Emit64(Gen, 0x0f48c0950fc08548);
            Emit16(Gen, 0xc0b6);
        } break;

        case NodeKind_LogicalOr:
        {
            gen_label* SkipRight = AllocateLabel();

            GenerateNode(Gen, Node->Left);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 85 Rel32  jnz SkipRight
            Emit40(Gen, 0x850fc08548);
            EmitRel32(Gen, SkipRight);

            GenerateNode(Gen, Node->Right);

            PlaceLabel(Gen, SkipRight);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 95 c0     setnz al
            // 48 0f b6 c0  movzx rax, al
            Emit64(Gen, 0x0f48c0950fc08548);
            Emit16(Gen, 0xc0b6);
        } break;

        case NodeKind_Ternary:
        {
            gen_label* SkipThen = AllocateLabel();
            gen_label* SkipElse = AllocateLabel();

            GenerateNode(Gen, Node->IfCond);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 84 Rel32  jz SkipThen
            Emit40(Gen, 0x840fc08548);
            EmitRel32(Gen, SkipThen);

            GenerateNode(Gen, Node->IfThen);

            // NOTE(vak):
            // e9 Rel32     jmp SkipElse
            Emit8(Gen, 0xe9);
            EmitRel32(Gen, SkipElse);

            PlaceLabel(Gen, SkipThen);
            GenerateNode(Gen, Node->IfElse);
            PlaceLabel(Gen, SkipElse);
        } break;

        case NodeKind_Assign:
        {
            GenerateAddress(Gen, Node->Left);
            Emit8(Gen, 0x50); // NOTE(vak): 50 push rax
            GenerateNode(Gen, Node->Right);
            Emit8(Gen, 0x59); // NOTE(vak): 59 pop rcx

            // NOTE(vak):
            // 48 89 01     mov [rcx], rax
            Emit24(Gen, 0x018948);
        } break;
    }
}

local usize Generate(void* Buffer, usize BufferSize, node* RootNode)
{
    gen_buffer Gen =
    {
        .Base = Buffer,
        .Size = BufferSize,
    };

    // NOTE(vak):
    // 55               push rbp
    // 48 8b ec         mov rbp, rsp
    // 48 81 ec Imm32   sub rsp, Gen.StackSize
    Emit56(&Gen, 0xec8148ec8b4855);

    s32* WriteStackSize = (s32*)(Gen.Base + Gen.Used);
    Emit32(&Gen, 0x00000000);

    for (
        node* Statement = RootNode;
        Statement;
        Statement = Statement->Next
    )
    {
        GenerateNode(&Gen, Statement);
    }

    // NOTE(vak):
    // 48 8b e5     mov rsp, rbp
    // 5d           pop rbp
    // c3           ret
    Emit40(&Gen, 0xc35de58b48);

    // NOTE(vak): Write in stack size

    if (Gen.Size)
    {
        if (Gen.StackSize > S32Max)
        {
            Println(Str("ERROR: stack size exceeds 32-bit signed integer max"));
            Exit(1);
        }

        *WriteStackSize = Gen.StackSize;
    }

    // NOTE(vak): Fill in all rel32 displacements

    for (
        gen_rel32* Rel32 = Gen.FirstRel32;
        Rel32;
        Rel32 = Rel32->Next
    )
    {
        gen_label* Label = Rel32->Target;

        if (Label->Offset == InvalidLabelOffset)
        {
            Println(Str("ERROR: rel32 target label is invalid (not placed)"));
            Exit(1);
        }

        ssize Disp = (ssize)Label->Offset - ((ssize)Rel32->Offset + 4);

        if ((Disp < S32Min) || (Disp > S32Max))
        {
            Println(Str("ERROR: rel32 displacement is outside 32-bit signed integer boundaries"));
            Exit(1);
        }

        if (Gen.Size)
        {
            s32* WriteAt = (s32*)(Gen.Base + Rel32->Offset);
            *WriteAt = (s32)Disp;
        }
    }

    return (Gen.Used);
}

