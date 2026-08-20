
// NOTE(vak): Assembly code generator

//      + Follows right after the Parser and directly emits x64 assembly code
//        from nodes.

//      + Currently, it generates inefficient x64 assembly code which
//        utilizes only three registers for evaluation: RAX, RCX, and RDX.

//      + The assembly code it generates follow a stack-machine style
//        of evaluation. Results are always stored in RAX, and if a
//        node wishes to save a result then it must push RAX to the
//        stack and pop it back into some other register (RCX, RDX, ...).
//        Once an operation is finished, it is expected to move the result
//        into RAX.

//      + The code generator supports labels and REL32 offsets. REL32 offsets
//        are reserved 32-bit offsets in the code that acts as a signed displacement
//        a label. A label can be allocated, and its offset can be placed at any
//        arbitrary point within the code. After code generation is finished,
//        the code generator will go through all REL32 offsets and fill in the
//        compute displacements.

//      + Symbols are managed via a linked list that also acts as a stack.
//        When a new symbol is added, it will be added at the end of the list.
//        When entering a scope, the current last symbol is recorded as a
//        restore point. Upon exiting the scope, everything from the restore
//        point onwards will be "popped" off the symbol linked list and onto
//        the free list, thereby removing it entirely.
//        See BeginScope() and EndScope() for more details.

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

typedef enum
{
    Gen_SymbolKind_Variable = 0,
    Gen_SymbolKind_Function,
} gen_symbol_kind;

typedef struct gen_symbol gen_symbol;
struct gen_symbol
{
    gen_symbol_kind Kind;
    string Name;
    type_spec TypeSpec;
    usize StackOffset;
    gen_label* FunctionCall;
    gen_symbol* Next;
};

typedef struct
{
    gen_symbol* RestoreTo;
} gen_scope;

typedef struct
{
    u8* Base;
    usize Used;
    usize StackSize;

    gen_rel32* FirstRel32;
    gen_rel32* LastRel32;

    gen_symbol* FirstSymbol;
    gen_symbol* LastSymbol;
    gen_symbol* FirstFreeSymbol;

    gen_label* BreakLabel;
    gen_label* ContinueLabel;
    gen_label* ReturnLabel;
} gen_buffer;

local arena_id CodeArenaID = NilArenaID;   // NOTE(vak): Machine code storage
local arena_id LabelArenaID = NilArenaID;  // NOTE(vak): Array of gen_label
local arena_id Rel32ArenaID = NilArenaID;  // NOTE(vak): Array of gen_rel32
local arena_id SymbolArenaID = NilArenaID; // NOTE(vak): Array of gen_symbol

local void EmitBytes(gen_buffer* Buffer, void* Bytes, usize Size)
{
    void* WriteAt = PushArenaSize(CodeArenaID, Size);

    CopyMemory(WriteAt, Bytes, Size);
    Buffer->Used += Size;
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
    if (!LabelArenaID)
        LabelArenaID = CreateArena(MB(1), GB(16));

    gen_label* Label = PushArena(LabelArenaID, gen_label);
    Label->Offset = InvalidLabelOffset;
    return (Label);
}

local void PlaceLabel(gen_buffer* Gen, gen_label* Label)
{
    Label->Offset = Gen->Used;
}

local void EmitRel32(gen_buffer* Gen, gen_label* Target)
{
    if (!Rel32ArenaID)
        Rel32ArenaID = CreateArena(MB(1), GB(16));

    gen_rel32* Rel32 = PushArena(Rel32ArenaID, gen_rel32);
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

local gen_symbol* AddSymbol(gen_buffer* Gen, gen_symbol_kind SymbolKind, string Name, type_spec TypeSpec)
{
    if (!SymbolArenaID)
        SymbolArenaID = CreateArena(MB(1), GB(16));

    gen_symbol* Symbol = Gen->FirstFreeSymbol;

    if (!Symbol)
    {
        Symbol = PushArena(SymbolArenaID, gen_symbol);
    }
    else
    {
        Gen->FirstFreeSymbol = Symbol->Next;
    }

    ZeroType(Symbol);

    Symbol->Kind = SymbolKind;
    Symbol->Name = Name;
    Symbol->TypeSpec = TypeSpec;

    if (SymbolKind == Gen_SymbolKind_Variable)
    {
        usize SizeToBeAllocated = 0;

        if (TypeSpec.ArrayCount)
        {
            usize ElementCount = 1;
            usize ElementSize = 0;

            type_spec* Scan = &TypeSpec;
            for (;;)
            {
                if (!Scan->ArrayCount)
                {
                    ElementSize = Scan->Bytes;
                    break;
                }
                else
                {
                    ElementCount *= Scan->ArrayCount;
                    Scan = Scan->PointingTo;
                }
            }

            SizeToBeAllocated = ElementCount * ElementSize;
        }
        else
            SizeToBeAllocated = TypeSpec.Bytes;

        Gen->StackSize += SizeToBeAllocated;
        Symbol->StackOffset = Gen->StackSize;
    }

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

local gen_symbol* LookupSymbol(gen_buffer* Gen, gen_symbol_kind SymbolKind, string Name)
{
    gen_symbol* Result = 0;

    for (
        gen_symbol* Symbol = Gen->FirstSymbol;
        Symbol;
        Symbol = Symbol->Next
    )
    {
        if (Symbol->Kind != SymbolKind)
            continue;

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

// NOTE(vak): Perform type checks, returns true if valid,
// else returns false.

// NOTE(vak):
//      EnforceSameSign:
//          + true:  Both sides must have the same sign
//          + false: Signs can differ
//
//      Promotable:
//          + true:  Valid if Left->Bytes >= Right->Bytes (promotable to equal to bigger size)
//          + false: Both sides must match in terms of size
//
//      SizeDoesntMatter:
//          + true:  Ignore any checks related to size
//          + false: Enable checks related to size

local b32 PerformTypeCheck(
    type_spec* Left, type_spec* Right,
    b32 EnforceSameSign,
    b32 Promotable,
    b32 SizeDoesntMatter
)
{
    if (!Left || !Right)
        return (false);

    if (Left->Kind != Right->Kind)
        return (false);

    if (Left->PointingTo || Right->PointingTo)
    {
        // NOTE(vak): Pointers should be absolutely identical
        return PerformTypeCheck(Left->PointingTo, Right->PointingTo, true, false, false);
    }

    b32 Result = true;

    if (EnforceSameSign)
    {
        Result &=
            (Left->Signed == Right->Signed) ||
            (Left->SignDoesntMatter) ||
            (Right->SignDoesntMatter);
    }

    if (!SizeDoesntMatter)
    {
        if (Promotable)
            Result &= (Left->Bytes >= Right->Bytes);
        else
            Result &= (Left->Bytes == Right->Bytes);
    }

    return (Result);
}

local type_spec GenerateNode(gen_buffer* Gen, node* Node);

local type_spec GenerateAddress(gen_buffer* Gen, node* Node)
{
    type_spec ResultType = {0};

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
            gen_symbol* Symbol = LookupSymbol(Gen, Gen_SymbolKind_Variable, Node->Identifier);
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

            ResultType.Signed = false;
            ResultType.Bytes = 8;
            ResultType.PointingTo = &Symbol->TypeSpec;
        } break;
    }

    return (ResultType);
}

// NOTE(vak):
// Memory address assumed to be in RAX.
// Resulting loaded value is put into RCX.
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

// NOTE(vak):
// Resulting memory address is put into RAX.
// Resulting loaded value is put into RCX.
local type_spec GenerateLoad(gen_buffer* Gen, node* Node)
{
    type_spec ResultType = {0};

    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: unimplemented node kind in GenerateLoad"));
            Exit(1);
        } break;

        case NodeKind_Dereference:
        {
            Emit8(Gen, 0x51); // NOTE(vak): push rcx

            type_spec TypeSpec = GenerateNode(Gen, Node->Left);
            if (!TypeSpec.PointingTo)
            {
                Println(Str("ERROR: trying to dereference a non-pointer"));
                Exit(1);
            }

            Emit8(Gen, 0x59); // NOTE(vak): pop rcx
            GenerateLoadForType(Gen, TypeSpec.PointingTo);

            ResultType = *TypeSpec.PointingTo;
        } break;

        case NodeKind_Identifier:
        {
            gen_symbol* Symbol = LookupSymbol(Gen, Gen_SymbolKind_Variable, Node->Identifier);
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

            // NOTE(vak): Arrays are treated as pointer addresses.
            // Otherwise just load in the variable value.

            if (TypeSpec->ArrayCount == 0)
                GenerateLoadForType(Gen, TypeSpec);
            else
                Emit24(Gen, 0xc88b48); // NOTE(vak): 48 8b c8 mov rcx, rax

            ResultType = *TypeSpec;
        } break;
    }

    return (ResultType);
}

// NOTE(vak):
// Destination memory address is assumed to be in RAX.
// Value to store is assumed to be in RCX.
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

// NOTE(vak): Value to store is assumed to be in RCX.
// Memory address will be generated in RAX.
local void GenerateStore(gen_buffer* Gen, node* Node, type_spec StoreType)
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
            Emit8(Gen, 0x51); // NOTE(vak): push rcx

            type_spec TypeSpec = GenerateNode(Gen, Node->Left);

            Emit8(Gen, 0x59); // NOTE(vak): pop rcx
 
            if (!PerformTypeCheck(TypeSpec.PointingTo, &StoreType, false, true, false))
            {
                Println(Str("ERROR: incompatible types"));
                Exit(1);
            }
 
            GenerateStoreForType(Gen, TypeSpec.PointingTo);
        } break;

        case NodeKind_Identifier:
        {
            gen_symbol* Symbol = LookupSymbol(Gen, Gen_SymbolKind_Variable, Node->Identifier);
            if (!Symbol)
            {
                Print(Str("ERROR: undeclared identifier '"));
                Print(Node->Identifier);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            type_spec* TypeSpec = &Symbol->TypeSpec;

            if (!PerformTypeCheck(TypeSpec, &StoreType, false, true, false))
            {
                Println(Str("ERROR: incompatible types"));
                Exit(1);
            }

            GenerateAddress(Gen, Node);
            GenerateStoreForType(Gen, TypeSpec);
        } break;
    }
}

// NOTE(vak):
//      Result of code generated from node is always stored in RAX.
//
//      Currently, the generated code will perform evaluation like a stack-machine.
//      This is horribly inefficient, but simple to implement.

// TODO(vak):
//      + Perform type checking. This would also make it easier to implement
//        pointer arithmetic (we have no way to know if we're dealing with
//        pointers inside of expressions at the moment.)
//
//      + Generate an intermediate representation (IR) from nodes, and use IR to
//        generate assembly so we can perform instruction selection, scheduling along
//        with register allocation. This would also mean that we can move away from the
//        current stack-machine style of evaluation.

local type_spec GenerateNode(gen_buffer* Gen, node* Node)
{
    type_spec ResultType = {0};

    if (!Node)
        return (ResultType);

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

            if (Node->Integer <= U8Max)
            {
                ResultType.SignDoesntMatter = true;
                ResultType.Signed = true;
                ResultType.Bytes = 1;
            }
            else if (Node->Integer <= U16Max)
            {
                ResultType.SignDoesntMatter = true;
                ResultType.Signed = true;
                ResultType.Bytes = 2;
            }
            else if (Node->Integer <= U32Max)
            {
                ResultType.SignDoesntMatter = true;
                ResultType.Signed = true;
                ResultType.Bytes = 4;
            }
            else if (Node->Integer <= S64Max)
            {
                ResultType.SignDoesntMatter = true;
                ResultType.Signed = true;
                ResultType.Bytes = 8;
            }
            else
            {
                ResultType.Signed = false;
                ResultType.Bytes = 8;
            }
        } break;

        case NodeKind_Identifier:
        {
            ResultType = GenerateLoad(Gen, Node);

            // NOTE(vak):
            // 48 8b c1         mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_Negate:
        {
            ResultType = GenerateNode(Gen, Node->Left);

            if (!ResultType.Signed)
            {
                Println(Str("ERROR: negating an unsigned value"));
                Exit(1);
            }

            // NOTE(vak):
            // 48 f7 d8         neg rax
            Emit24(Gen, 0xd8f748);
        } break;

        case NodeKind_BitwiseNot:
        {
            ResultType = GenerateNode(Gen, Node->Left);

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

            ResultType.Signed = false;
            ResultType.Bytes = 1;
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

            type_spec RightType = GenerateNode(Gen, Node->Right);

            Emit8(Gen, 0x50); // NOTE(vak): 50 push rax

            type_spec LeftType = GenerateNode(Gen, Node->Left);

            Emit8(Gen, 0x59); // NOTE(vak): 59 pop rcx

            if (LeftType.PointingTo && RightType.PointingTo)
            {
                if (!PerformTypeCheck(&LeftType, &RightType, false, false, true))
                {
                    Println(Str("ERROR: incompatible types in expression"));
                    Exit(1);
                }
            }

            ResultType = (type_spec)
            {
                .Signed = LeftType.Signed || RightType.Signed,     // NOTE(vak): Inherit sign
                .Bytes = Maximum(LeftType.Bytes, RightType.Bytes), // NOTE(vak): Promote
            };

            b32 PointerArithmetic = (LeftType.PointingTo || RightType.PointingTo);

            if (0) {}
            else if (LeftType.PointingTo)  ResultType.PointingTo = LeftType.PointingTo;
            else if (RightType.PointingTo) ResultType.PointingTo = RightType.PointingTo;

            if (PointerArithmetic)
            {
                if ((Node->Kind != NodeKind_Add) && (Node->Kind != NodeKind_Sub))
                {
                    Println(Str("ERROR: invalid pointer arithmetic expression"));
                    Exit(1);
                }
            }

            switch (Node->Kind)
            {
                case NodeKind_Add:
                {
                    if (PointerArithmetic)
                    {
                        if (LeftType.PointingTo && RightType.PointingTo)
                        {
                        }
                        else if (LeftType.PointingTo)
                        {
                            // NOTE(vak):
                            // 48 69 c9 Imm32   imul rcx, rcx, LeftType.PointingTo->Bytes
                            Emit24(Gen, 0xc96948);
                            Emit32(Gen, LeftType.PointingTo->Bytes);
                        }
                        else if (RightType.PointingTo)
                        {
                            // NOTE(vak):
                            // 48 69 c0 Imm32   imul rax, rax, RightType.PointingTo->Bytes
                            Emit24(Gen, 0xc06948);
                            Emit32(Gen, RightType.PointingTo->Bytes);
                        }
                    }

                    // NOTE(vak):
                    // 48 03 c1         add rax, rcx
                    Emit24(Gen, 0xc10348); break;
                } break;

                // NOTE(vak):
                // 48 2b c1         sub rax, rcx
                case NodeKind_Sub:
                {
                    if (PointerArithmetic)
                    {
                        if (LeftType.PointingTo && RightType.PointingTo)
                        {
                        }
                        else if (LeftType.PointingTo)
                        {
                            // NOTE(vak):
                            // 48 69 c9 Imm32   imul rcx, rcx, LeftType.PointingTo->Bytes
                            Emit24(Gen, 0xc96948);
                            Emit32(Gen, LeftType.PointingTo->Bytes);
                        }
                        else if (RightType.PointingTo)
                        {
                            // NOTE(vak):
                            // 48 69 c0 Imm32   imul rax, rax, RightType.PointingTo->Bytes
                            Emit24(Gen, 0xc06948);
                            Emit32(Gen, RightType.PointingTo->Bytes);
                        }
                    }

                    Emit24(Gen, 0xc12b48);
                } break;

                case NodeKind_Mul:
                {
                    if (ResultType.Signed)
                    {
                        // NOTE(vak):
                        // 48 0f af c1      imul rax, rcx
                        Emit32(Gen, 0xc1af0f48);
                    }
                    else
                    {
                        // NOTE(vak):
                        // 48 99            cqo
                        // 48 f7 e1         mul rcx
                        Emit40(Gen, 0xe1f7489948);
                    }
                } break;

                case NodeKind_Div:
                {
                    if (ResultType.Signed)
                    {
                        // NOTE(vak):
                        // 48 99            cqo
                        // 48 f7 f9         idiv rcx
                        Emit40(Gen, 0xf9f7489948);
                    }
                    else
                    {
                        // NOTE(vak):
                        // 48 99            cqo
                        // 48 f7 f1         div rcx
                        Emit40(Gen, 0xf1f7489948);
                    }
                } break;

                case NodeKind_Mod:
                {
                    if (ResultType.Signed)
                    {
                        // NOTE(vak):
                        // 48 99            cqo
                        // 48 f7 f9         idiv rcx
                        Emit40(Gen, 0xf9f7489948);
                    }
                    else
                    {
                        // NOTE(vak):
                        // 48 99            cqo
                        // 48 f7 f1         div rcx
                        Emit40(Gen, 0xf1f7489948);
                    }

                    // NOTE(vak):
                    // 48 8b c2         mov rax, rdx
                    Emit24(Gen, 0xc28b48);
                } break;

                case NodeKind_ShiftLeft:
                {
                    // NOTE(vak): No need to differentiate between signed/unsigned
                    // for shift left since SAL/SHL are mnemonics for the
                    // same instruction.

                    Emit24(Gen, 0xe0d348); // NOTE(vak): 48 d3 e0 sal rax, cl
                } break;

                case NodeKind_ShiftRight:
                {
                    if (ResultType.Signed)
                        Emit24(Gen, 0xf8d348); // NOTE(vak): 48 d3 f8 sar rax, cl
                    else
                        Emit24(Gen, 0xe8d348); // NOTE(vak): 48 d3 e8 shr rax, cl
                } break;

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
            type_spec RightType = GenerateNode(Gen, Node->Right);

            Emit8(Gen, 0x50); // NOTE(vak): 50 push rax

            type_spec LeftType = GenerateNode(Gen, Node->Left);

            Emit8(Gen, 0x59); // NOTE(vak): 59 pop rcx

            if (!PerformTypeCheck(&LeftType, &RightType, true, false, true))
            {
                Println(Str("ERROR: incompatible types in comparison"));
                Exit(1);
            }

            ResultType.Signed = false;
            ResultType.Bytes = 1;

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

            ResultType.Signed = false;
            ResultType.Bytes = 1;
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

            ResultType.Signed = false;
            ResultType.Bytes = 1;
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

            type_spec ThenType = GenerateNode(Gen, Node->IfThen);

            // NOTE(vak):
            // e9 Rel32     jmp SkipElse
            Emit8(Gen, 0xe9);
            EmitRel32(Gen, SkipElse);

            PlaceLabel(Gen, SkipThen);
            type_spec ElseType = GenerateNode(Gen, Node->IfElse);
            PlaceLabel(Gen, SkipElse);

            ResultType = (type_spec)
            {
                .Signed = ThenType.Signed || ElseType.Signed,     // NOTE(vak): Inherit sign
                .Bytes = Maximum(ThenType.Bytes, ElseType.Bytes), // NOTE(vak): Promote
            };
        } break;

        case NodeKind_PostIncrement:
        {
            ResultType = GenerateLoad(Gen, Node->Left);

            Emit8(Gen, 0x51); // NOTE(vak): 51 push rcx
            if (!ResultType.PointingTo)
            {
                // NOTE(vak):
                // 48 ff c1     inc rcx
                Emit24(Gen, 0xc1ff48);
            }
            else
            {
                // NOTE(vak):
                // 48 81 c1 Imm32   add rcx, ResultType.PointingTo->Bytes)
                Emit24(Gen, 0xc18148);
                Emit32(Gen, ResultType.PointingTo->Bytes);
            }

            GenerateStore(Gen, Node->Left, ResultType);

            // NOTE(vak):
            // 58           pop rax
            Emit8(Gen, 0x58);
        } break;

        case NodeKind_PostDecrement:
        {
            ResultType = GenerateLoad(Gen, Node->Left);

            Emit8(Gen, 0x51); // NOTE(vak): 51 push rcx
            if (!ResultType.PointingTo)
            {
                // NOTE(vak):
                // 48 ff c9     dec rcx
                Emit32(Gen, 0xc9ff4851);
            }
            else
            {
                // NOTE(vak):
                // 48 81 e9 Imm32   sub rcx, ResultType.PointingTo->Bytes)
                Emit24(Gen, 0xe98148);
                Emit32(Gen, ResultType.PointingTo->Bytes);
            }

            GenerateStore(Gen, Node->Left, ResultType);

            // NOTE(vak):
            // 58           pop rax
            Emit8(Gen, 0x58);
        } break;

        case NodeKind_PreIncrement:
        {
            ResultType = GenerateLoad(Gen, Node->Left);

            if (!ResultType.PointingTo)
            {
                // NOTE(vak):
                // 48 ff c1     inc rcx
                Emit24(Gen, 0xc1ff48);
            }
            else
            {
                // NOTE(vak):
                // 48 81 c1 Imm32   add rcx, ResultType.PointingTo->Bytes)
                Emit24(Gen, 0xc18148);
                Emit32(Gen, ResultType.PointingTo->Bytes);
            }

            GenerateStore(Gen, Node->Left, ResultType);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_PreDecrement:
        {
            ResultType = GenerateLoad(Gen, Node->Left);

            if (!ResultType.PointingTo)
            {
                // NOTE(vak):
                // 48 ff c9     dec rcx
                Emit32(Gen, 0xc9ff4851);
            }
            else
            {
                // NOTE(vak):
                // 48 81 e9 Imm32   sub rcx, ResultType.PointingTo->Bytes)
                Emit24(Gen, 0xe98148);
                Emit32(Gen, ResultType.PointingTo->Bytes);
            }

            GenerateStore(Gen, Node->Left, ResultType);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_AddressOf:
        {
            ResultType = GenerateAddress(Gen, Node->Left);
        } break;

        case NodeKind_Dereference:
        {
            ResultType = GenerateLoad(Gen, Node);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_Assign:
        {
            type_spec RightType = GenerateNode(Gen, Node->Right);

            // NOTE(vak):
            // 48 8b c8     mov rcx, rax
            Emit24(Gen, 0xc88b48);

            GenerateStore(Gen, Node->Left, RightType);

            // NOTE(vak):
            // 48 8b c1     mov rax, rcx
            Emit24(Gen, 0xc18b48);
        } break;

        case NodeKind_Declare:
        {
            type_spec TypeSpec = Node->TypeSpec;
            string Name = Node->Identifier;

            if (TypeSpec.Kind == TypeSpecKind_Function)
            {
                Println(Str("ERROR: function declaration inside function body is not allowed"));
                Exit(1);
            }

            gen_symbol* Symbol = LookupSymbol(Gen, Gen_SymbolKind_Variable, Name);
            if (Symbol)
            {
                Print(Str("ERROR: redeclaration of identifier '"));
                Print(Name);
                Print(Str("'"));
                PrintNewLine();
                Exit(1);
            }

            AddSymbol(Gen, Gen_SymbolKind_Variable, Name, TypeSpec);

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

    return (ResultType);
}

local void GenerateFunctionBody(gen_buffer* Gen, gen_symbol* FunctionSymbol, node* FunctionBody)
{
    Gen->ReturnLabel = AllocateLabel();
    Gen->StackSize = 0;

    if (FunctionSymbol)
    {
        FunctionSymbol->FunctionCall = AllocateLabel();
        PlaceLabel(Gen, FunctionSymbol->FunctionCall);
    }

    // NOTE(vak): Prologue
    // 55               push rbp
    // 48 8b ec         mov rbp, rsp
    // 48 81 ec Imm32   sub rsp, Gen.StackSize
    Emit56(Gen, 0xec8148ec8b4855);

    s32* WriteStackSize = (s32*)(Gen->Base + Gen->Used);
    Emit32(Gen, 0x00000000);

    for (
        node* Statement = FunctionBody;
        Statement;
        Statement = Statement->Next)
    {
        GenerateNode(Gen, Statement);
    }

    // NOTE(vak): Epilogue
    // 48 8b e5     mov rsp, rbp
    // 5d           pop rbp
    // c3           ret
    Emit40(Gen, 0xc35de58b48);

    // NOTE(vak): Write in stack size

    if (Gen->StackSize > S32Max)
    {
        Println(Str("ERROR: stack size exceeds 32-bit signed integer max"));
        Exit(1);
    }

    *WriteStackSize = Gen->StackSize;

    Gen->ReturnLabel = 0;
}

local void GenerateTopLevel(gen_buffer* Gen, node* Node)
{
    switch (Node->Kind)
    {
        default:
        {
            Println(Str("ERROR: invalid statement that is outside of a function"));
            Exit(1);
        } break;

        case NodeKind_Declare:
        {
            type_spec TypeSpec = Node->TypeSpec;
            string Name = Node->Identifier;

            if (TypeSpec.Kind == TypeSpecKind_Normal)
            {
                Println(Str("ERROR: variable declaration outside function body is not implemented yet"));
                Exit(1);
            }

            gen_symbol* Symbol = LookupSymbol(Gen, Gen_SymbolKind_Function, Name);
            if (!Symbol)
            {
                Symbol = AddSymbol(Gen, Gen_SymbolKind_Function, Name, TypeSpec);
            }
            else
            {
                if (!PerformTypeCheck(Symbol->TypeSpec.ReturnType, TypeSpec.ReturnType, true, false, false))
                {
                    Println(Str("ERROR: function return type differs from previous declaration"));
                    Exit(1);
                }
            }

            if (Node->FunctionBody)
            {
                if (Symbol->FunctionCall)
                {
                    Println(Str("ERROR: redefinition of function body"));
                    Exit(1);
                }

                GenerateFunctionBody(Gen, Symbol, Node->FunctionBody);
            }
        } break;
    }
}

typedef struct
{
    void* MachineCode;  // NOTE(vak): Base address of generated machine code
    usize EntryOffset;  // NOTE(vak): Entry point byte offset into the buffer
    usize CodeSize;     // NOTE(vak): Size of generated code in bytes.
} gen_result;

// NOTE(vak): Generates complete x64 assembly code from a parsed program.

//      Buffer, BufferSize:
//              + If both are set to 0, then code generator will not emit code, and instead
//                return the CodeSize that will be generated.
//              + Otherwise, code generator will emit code. If BufferSize is not large enough
//                then code generator will stop emitting code at the end of the buffer.

//      RootNode:
//              + Root node that is obtained from the parser

//      MainFunctionName:
//              + if NilString: No main function specified, treat RootNode as if it were inside a function
//                              and generate.
//              + else:         Main function name is specified, so look for functions and generate them.
//                              After generation, look for the main function and set the entry point offset.

local gen_result Generate(node* RootNode, string MainFunctionName)
{
    if (!CodeArenaID)
        CodeArenaID = CreateArena(MB(1), GB(16));

    gen_buffer Gen =
    {
        .Base = GetArenaAllocationPointer(CodeArenaID),
        .Used = 0,
    };

    gen_result Result =
    {
        .MachineCode = Gen.Base,
    };

    if (MainFunctionName.Size)
    {
        // NOTE(vak): Main function specified, so generate functions

        for (
            node* Statement = RootNode;
            Statement;
            Statement = Statement->Next
        )
        {
            GenerateTopLevel(&Gen, Statement);
        }

        gen_symbol* MainFunctionSymbol = LookupSymbol(&Gen, Gen_SymbolKind_Function, MainFunctionName);
        if (!MainFunctionSymbol)
        {
            Print(Str("ERROR: cannot find entry point '"));
            Print(MainFunctionName);
            Print(Str("'"));
            PrintNewLine();
            Exit(1);
        }

        if (!MainFunctionSymbol->FunctionCall)
        {
            Println(Str("ERROR: entry point is declared but not defined"));
            Exit(1);
        }

        Result.EntryOffset = MainFunctionSymbol->FunctionCall->Offset;
    }
    else
    {
        // NOTE(vak): No main function specified, treat code as if it were
        // inside a function and generate. Entry offset is always placed at
        // the start of the buffer (EntryOffset = 0), so there is no need to
        // set it.

        GenerateFunctionBody(&Gen, 0, RootNode);
    }

    Result.CodeSize = Gen.Used;

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

        s32* WriteAt = (s32*)(Gen.Base + Rel32->Offset);
        *WriteAt = (s32)Disp;
    }

    return (Result);
}

