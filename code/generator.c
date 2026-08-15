
#pragma once

typedef struct
{
    u8* Base;
    usize Size;
    usize Used;
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

        case NodeKind_Add:
        case NodeKind_Sub:
        case NodeKind_Mul:
        case NodeKind_Div:
        case NodeKind_Mod:
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
    }
}

local usize Generate(void* Buffer, usize BufferSize, node* RootNode)
{
    gen_buffer Gen =
    {
        .Base = Buffer,
        .Size = BufferSize,
        .Used = 0,
    };

    // NOTE(vak):
    // 55           push rbp
    // 48 8b ec     mov rbp, rsp
    Emit32(&Gen, 0xec8b4855);

    GenerateNode(&Gen, RootNode);

    // NOTE(vak):
    // 48 8b e5     mov rsp, rbp
    // 5d           pop rbp
    // c3           ret
    Emit40(&Gen, 0xc35de58b48);

    return (Gen.Used);
}

