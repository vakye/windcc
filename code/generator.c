
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
    type_spec TypeSpec;
    gen_symbol* Next;
};

typedef struct
{
    gen_symbol* RestoreTo;
} gen_scope;

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
    gen_symbol* FirstFreeSymbol;

    gen_label* BreakLabel;
    gen_label* ContinueLabel;
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

local gen_symbol* AddSymbol(gen_buffer* Gen, string Name, type_spec TypeSpec)
{
    gen_symbol* Symbol = Gen->FirstFreeSymbol;

    if (!Symbol)
    {
        Symbol = Allocate(sizeof(gen_symbol));
    }
    else
    {
        Gen->FirstFreeSymbol = Symbol->Next;
    }

    Gen->StackSize += TypeSpec.Bytes;

    ZeroType(Symbol);
    Symbol->Name = Name;
    Symbol->StackOffset = Gen->StackSize;
    Symbol->TypeSpec = TypeSpec;

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

local gen_scope BeginScope(gen_buffer* Gen)
{
    gen_scope Scope =
    {
        .RestoreTo = Gen->LastSymbol,
    };

    return (Scope);
}

local void EndScope(gen_buffer* Gen, gen_scope Scope)
{
    gen_symbol* First = Scope.RestoreTo->Next;
    gen_symbol* Last = Gen->LastSymbol;

    if (First)
    {
        Gen->LastSymbol = Scope.RestoreTo;
        Gen->LastSymbol->Next = 0;

        Last->Next = Gen->FirstFreeSymbol;
        Gen->FirstFreeSymbol = First;
    }
}

local void GenerateNode(gen_buffer* Gen, node* Node);

local void GenerateAddress(gen_buffer* Gen, node* Node)
{
    if (!Node)
    {
        Println(Str("ERROR: no node passed into GenerateAddress"));
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
                Print(Str("ERROR: undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
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

local void GenerateLoadForType(gen_buffer* Gen, type_spec* TypeSpec)
{
    if (TypeSpec->Signed)
    {
        switch (TypeSpec->Bytes)
        {
            default: Println(Str("ERROR: invalid TypeSpec.Bytes")); Exit(1); break;

            case 1: Emit32(Gen, 0x08be0f48);    break; // NOTE(vak): 48 0f be 08   movsx rcx, byte [rax]
            case 2: Emit32(Gen, 0x08bf0f48);    break; // NOTE(vak): 48 0f bf 08   movsx rcx, word [rax]
            case 4: Emit24(Gen, 0x086348);      break; // NOTE(vak): 48 63 08      movsxd rcx, dword [rax]
            case 8: Emit24(Gen, 0x088b48);      break; // NOTE(vak): 48 8b 08      mov rcx, [rax]
        }
    }
    else
    {
        switch (TypeSpec->Bytes)
        {
            default: Println(Str("ERROR: invalid TypeSpec.Bytes")); Exit(1); break;

            // NOTE(vak): Operations involving 32-bit registers in x64 automatically zeroes
            // upper 32-bit part of register so there is using a 32-bit load is okay.

            case 1: Emit32(Gen, 0x08b60f48);    break; // NOTE(vak): 48 0f b6 08   movzx rcx, byte [rax]
            case 2: Emit32(Gen, 0x08b70f48);    break; // NOTE(vak): 48 0f b7 08   movzx rcx, word [rax]
            case 4: Emit16(Gen, 0x088b);        break; // NOTE(vak): 8b 00         mov ecx, [rax]
            case 8: Emit24(Gen, 0x088b48);      break; // NOTE(vak): 48 8b 08      mov rcx, [rax]
        }
    }
}

local type_spec* ObtainDereferenceType(gen_buffer* Gen, node* Node)
{
    type_spec* Type = 0;

    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: unimplemented node kind in ObtainDereferenceType"));
            Exit(1);
        } break;

        case NodeKind_Dereference:
        {
            Type = ObtainDereferenceType(Gen, Node->Left);

            if (!Type->PointingTo)
            {
                Println(Str("ERROR: dereferencing a non-pointer"));
                Exit(1);
            }

            Type = Type->PointingTo;
        } break;

        case NodeKind_AddressOf:
        {
            node* Left = Node->Left;

            if (Left->Kind != NodeKind_Identifier)
            {
                Println(Str("ERROR: invalid address-of operation"));
                Exit(1);
            }

            gen_symbol* Symbol = LookupSymbol(Gen, Left->Identifier);
            if (!Symbol)
            {
                Print(Str("ERROR: undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            Type = &Symbol->TypeSpec;
        } break;

        case NodeKind_Identifier:
        {
            gen_symbol* Symbol = LookupSymbol(Gen, Node->Identifier);
            if (!Symbol)
            {
                Print(Str("ERROR: undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            if (!Symbol->TypeSpec.PointingTo)
            {
                Println(Str("ERROR: dereferencing a non-pointer"));
                Exit(1);
            }

            Type = Symbol->TypeSpec.PointingTo;
        } break;
    }

    return (Type);
}

// NOTE(vak): Load value of node: RAX=MemoryAddress, RCX=LoadedValue
local void GenerateLoad(gen_buffer* Gen, node* Node)
{
    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: unimplemented node kind in GenerateLoad"));
            Exit(1);
        } break;

        case NodeKind_Dereference:
        {
            type_spec* TypeSpec = ObtainDereferenceType(Gen, Node->Left);

            if (!TypeSpec)
            {
                Println(Str("ERROR: invalid dereference"));
                Exit(1);
            }

            Emit8(Gen, 0x51); // NOTE(vak): push rcx
            GenerateNode(Gen, Node->Left);
            Emit8(Gen, 0x59); // NOTE(vak): pop rcx
            GenerateLoadForType(Gen, TypeSpec);
        } break;

        case NodeKind_Identifier:
        {
            gen_symbol* Symbol = LookupSymbol(Gen, Node->Identifier);
            if (!Symbol)
            {
                Print(Str("ERROR: undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            type_spec* TypeSpec = &Symbol->TypeSpec;

            GenerateAddress(Gen, Node);
            GenerateLoadForType(Gen, TypeSpec);
        } break;
    }
}

local void GenerateStoreForType(gen_buffer* Gen, type_spec* TypeSpec)
{
    switch (TypeSpec->Bytes)
    {
        default: Println(Str("ERROR: invalid TypeSpec.Bytes")); Exit(1); break;

        case 1: Emit16(Gen, 0x0888);    break; // NOTE(vak): 88 08      mov [rax], cl
        case 2: Emit24(Gen, 0x088966);  break; // NOTE(vak): 66 89 08   mov [rax], cx
        case 4: Emit16(Gen, 0x0889);    break; // NOTE(vak): 89 08      mov [rax], ecx
        case 8: Emit24(Gen, 0x088948);  break; // NOTE(vak): 48 89 08   mov [rax], rcx
    }
}

// NOTE(vak): Store value of node: RAX=MemoryAddress, RCX=StoreValue
local void GenerateStore(gen_buffer* Gen, node* Node)
{
    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: unimplemented node kind in GenerateStore"));
            Exit(1);
        } break;

        case NodeKind_Dereference:
        {
            type_spec* TypeSpec = ObtainDereferenceType(Gen, Node->Left);

            if (!TypeSpec)
            {
                Println(Str("ERROR: invalid dereference"));
                Exit(1);
            }
 
            Emit8(Gen, 0x51); // NOTE(vak): push rcx
            GenerateNode(Gen, Node->Left);
            Emit8(Gen, 0x59); // NOTE(vak): pop rcx
            GenerateStoreForType(Gen, TypeSpec);
        } break;

        case NodeKind_Identifier:
        {
            gen_symbol* Symbol = LookupSymbol(Gen, Node->Identifier);
            if (!Symbol)
            {
                Print(Str("ERROR: undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            type_spec* TypeSpec = &Symbol->TypeSpec;

            GenerateAddress(Gen, Node);
            GenerateStoreForType(Gen, TypeSpec);
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

        case NodeKind_Block:
        {
            gen_scope Scope = BeginScope(Gen);

            for (
                node* Statement = Node->FirstStatement;
                Statement;
                Statement = Statement->Next
            )
            {
                GenerateNode(Gen, Statement);
            }

            EndScope(Gen, Scope);
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
            GenerateLoad(Gen, Node);

            // NOTE(vak):
            // 48 8b c1         mov rax, rcx
            Emit24(Gen, 0xc18b48);
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

        case NodeKind_PostIncrement:
        {
            GenerateLoad(Gen, Node->Left);

            // NOTE(vak):
            // 51           push rcx
            // 48 ff c1     inc rcx
            Emit32(Gen, 0xc1ff4851);

            GenerateStore(Gen, Node->Left);

            // NOTE(vak):
            // 58           pop rax
            Emit8(Gen, 0x58);
        } break;

        case NodeKind_PostDecrement:
        {
            GenerateLoad(Gen, Node->Left);

            // NOTE(vak):
            // 51           push rcx
            // 48 ff c9     dec rcx
            Emit32(Gen, 0xc9ff4851);

            GenerateStore(Gen, Node->Left);

            // NOTE(vak):
            // 58           pop rax
            Emit8(Gen, 0x58);
        } break;

        case NodeKind_PreIncrement:
        {
            GenerateLoad(Gen, Node->Left);

            // NOTE(vak):
            // 48 ff c1     inc rcx
            Emit24(Gen, 0xc1ff48);

            GenerateStore(Gen, Node->Left);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_PreDecrement:
        {
            GenerateLoad(Gen, Node->Left);

            // NOTE(vak):
            // 48 ff c9     dec rcx
            Emit24(Gen, 0xc9ff48);

            GenerateStore(Gen, Node->Left);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_AddressOf:
        {
            GenerateAddress(Gen, Node->Left);
        } break;

        case NodeKind_Dereference:
        {
            GenerateLoad(Gen, Node);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_Assign:
        {
            GenerateNode(Gen, Node->Right);

            // NOTE(vak):
            // 48 8b c8     mov rcx, rax
            Emit24(Gen, 0xc88b48);

            GenerateStore(Gen, Node->Left);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_Declare:
        {
            type_spec TypeSpec = Node->TypeSpec;
            string Name = Node->Identifier;

            gen_symbol* Symbol = LookupSymbol(Gen, Name);
            if (Symbol)
            {
                Print(Str("ERROR: redeclaration of identifier '"));
                Print(Name);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            AddSymbol(Gen, Name, TypeSpec);

            GenerateNode(Gen, Node->Initializer);
        } break;

        case NodeKind_If:
        {
            gen_label* SkipThen = AllocateLabel();

            GenerateNode(Gen, Node->IfCond);

            // NOTE(vak):
            // 48 85 c0     test rax, rax
            // 0f 84 Rel32  jz SkipThen
            Emit40(Gen, 0x840fc08548);
            EmitRel32(Gen, SkipThen);

            GenerateNode(Gen, Node->IfThen);

            if (Node->IfElse)
            {
                gen_label* SkipElse = AllocateLabel();

                // NOTE(vak):
                // e9 Rel32     jmp SkipElse
                Emit8(Gen, 0xe9);
                EmitRel32(Gen, SkipElse);

                PlaceLabel(Gen, SkipThen);
                GenerateNode(Gen, Node->IfElse);
                PlaceLabel(Gen, SkipElse);
            }
            else
            {
                PlaceLabel(Gen, SkipThen);
            }
        } break;

        case NodeKind_For:
        {
            gen_label* StartOfLoop = AllocateLabel();
            gen_label* EndOfLoop = AllocateLabel();

            gen_label* OldBreakLabel = Gen->BreakLabel;
            gen_label* OldContinueLabel = Gen->ContinueLabel;

            gen_label* ContinueLabel = AllocateLabel();

            Gen->BreakLabel = EndOfLoop;
            Gen->ContinueLabel = ContinueLabel;

            gen_scope Scope = BeginScope(Gen);

            GenerateNode(Gen, Node->ForInit);

            PlaceLabel(Gen, StartOfLoop);

            if (Node->ForCond)
            {
                GenerateNode(Gen, Node->ForCond);

                // NOTE(vak):
                // 48 85 c0     test rax, rax
                // 0f 84 Rel32  jz EndOfLoop
                Emit40(Gen, 0x840fc08548);
                EmitRel32(Gen, EndOfLoop);
            }

            GenerateNode(Gen, Node->ForBody);
            PlaceLabel(Gen, ContinueLabel);
            GenerateNode(Gen, Node->ForIter);

            // NOTE(vak):
            // e9 Rel32     jmp StartOfLoop
            Emit8(Gen, 0xe9);
            EmitRel32(Gen, StartOfLoop);

            PlaceLabel(Gen, EndOfLoop);

            EndScope(Gen, Scope);

            Gen->BreakLabel = OldBreakLabel;
            Gen->ContinueLabel = OldContinueLabel;
        } break;

        case NodeKind_Break:
        {
            if (!Gen->BreakLabel)
            {
                Println(Str("ERROR: invalid break statement"));
                Exit(1);
            }

            // NOTE(vak):
            // e9 Rel32     jmp BreakLabel
            Emit8(Gen, 0xe9);
            EmitRel32(Gen, Gen->BreakLabel);
        } break;

        case NodeKind_Continue:
        {
            if (!Gen->ContinueLabel)
            {
                Println(Str("ERROR: invalid continue statement"));
                Exit(1);
            }

            // NOTE(vak):
            // e9 Rel32     jmp ContinueLabel
            Emit8(Gen, 0xe9);
            EmitRel32(Gen, Gen->ContinueLabel);
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

