
// NOTE(vak): References:
//      + https://openmlsys.github.io/html-en/chapter_compiler_frontend/Intermediate_Representation.html#hybrid-intermediate-representation

#pragma once

typedef enum
{
    IR_OperandKind_Nil = 0,
    IR_OperandKind_Imm,     // NOTE(vak): Immediate value
    IR_OperandKind_Temp,    // NOTE(vak): Temporary value
} ir_operand_kind;

typedef struct
{
    ir_operand_kind Kind;

    union
    {
        struct { usize Value; } Imm;
        struct { u32 Index; } Temp;
    };
} ir_operand;

#define IR_NilOperand()         (ir_operand){0}
#define IR_ImmOperand(Integer)  (ir_operand){IR_OperandKind_Imm,  .Imm  = {Integer}}
#define IR_TempOperand(Index)   (ir_operand){IR_OperandKind_Temp, .Temp = {Index}}

#define AllOpCodesIR(X) \
    X(Nop) \
    X(Load64)                   /* NOTE(vak): Dest = Source1 */ \
    X(Add64)                    /* NOTE(vak): Dest = Source1 + Source2 */ \
    X(Sub64)                    /* NOTE(vak): Dest = Source1 - Source2 */ \
    X(MulI64)                   /* NOTE(vak): Dest = Source1 * Source2 */ \
    X(DivI64)                   /* NOTE(vak): Dest = Source1 / Source2 */ \
    X(ModI64)                   /* NOTE(vak): Dest = Source1 % Source2 */ \
    X(MulU64)                   /* NOTE(vak): Dest = Source1 * Source2 */ \
    X(DivU64)                   /* NOTE(vak): Dest = Source1 / Source2 */ \
    X(ModU64)                   /* NOTE(vak): Dest = Source1 % Source2 */ \
    X(Ret)                      /* NOTE(vak): ReturnValue = Source1, InstructionPointer = PopStack64() */ \

typedef enum
{
    #define DefineOpCodeIR(Name) IR_OpCode_##Name,

    AllOpCodesIR(DefineOpCodeIR)

    #undef DefineOpCodeIR
} ir_op_code;

typedef u32 ir_op_id;

typedef struct
{
    ir_op_code Code;
    ir_operand Dest;
    ir_operand Source1;
    ir_operand Source2;
} ir_op;

typedef struct
{
    ir_op_id    First;
    u32         Count;
    u32         TempRegisterCount;
} ir_block;

local ir_op OperationsIR[4096] = {0};
local u32 OperationCountIR = 0;

local ir_op_id PushOpIR(
    ir_op_code OpCode,
    ir_operand Dest,
    ir_operand Source1,
    ir_operand Source2
)
{
    if (OperationCountIR == ArrayCount(OperationsIR))
    {
        Println(Str("ERROR: too many IR ops"));
        Exit(1);
    }

    ir_op_id OpID = OperationCountIR++;

    ZeroType(OperationsIR + OpID);

    OperationsIR[OpID].Code     = OpCode;
    OperationsIR[OpID].Dest     = Dest;
    OperationsIR[OpID].Source1  = Source1;
    OperationsIR[OpID].Source2  = Source2;

    return (OpID);
}

#define IR_Load64(Dest, Source)             PushOpIR(IR_OpCode_Load64, Dest, Source, IR_NilOperand())
#define IR_Add64(Dest, Source1, Source2)    PushOpIR(IR_OpCode_Add64,  Dest, Source1, Source2)
#define IR_Sub64(Dest, Source1, Source2)    PushOpIR(IR_OpCode_Sub64,  Dest, Source1, Source2)
#define IR_MulI64(Dest, Source1, Source2)   PushOpIR(IR_OpCode_MulI64, Dest, Source1, Source2)
#define IR_DivI64(Dest, Source1, Source2)   PushOpIR(IR_OpCode_DivI64, Dest, Source1, Source2)
#define IR_ModI64(Dest, Source1, Source2)   PushOpIR(IR_OpCode_ModI64, Dest, Source1, Source2)
#define IR_MulU64(Dest, Source1, Source2)   PushOpIR(IR_OpCode_MulU64, Dest, Source1, Source2)
#define IR_DivU64(Dest, Source1, Source2)   PushOpIR(IR_OpCode_DivU64, Dest, Source1, Source2)
#define IR_ModU64(Dest, Source1, Source2)   PushOpIR(IR_OpCode_ModU64, Dest, Source1, Source2)
#define IR_Ret(Source)                      PushOpIR(IR_OpCode_Ret, IR_NilOperand(), Source, IR_NilOperand())

#define IR_IsImm(Operand)  ((Operand).Kind == IR_OperandKind_Imm)
#define IR_IsTemp(Operand) ((Operand).Kind == IR_OperandKind_Temp)

local u32 TempRegisterIndex = 0;

local ir_operand GenerateNodeIR(node_id NodeID)
{
    ir_operand Result = IR_NilOperand();

    node* Node = GetNode(NodeID);
    switch (Node->Kind)
    {
        default: break;

        case NodeKind_Integer:
        {
            Result = IR_ImmOperand(Node->Integer.Value);
        } break;

        case NodeKind_Add:
        {
            ir_operand Left = GenerateNodeIR(Node->Binary.Left);
            ir_operand Right = GenerateNodeIR(Node->Binary.Right);

            Result = IR_TempOperand(TempRegisterIndex++);

            IR_Add64(Result, Left, Right);
        } break;

        case NodeKind_Sub:
        {
            ir_operand Left = GenerateNodeIR(Node->Binary.Left);
            ir_operand Right = GenerateNodeIR(Node->Binary.Right);

            Result = IR_TempOperand(TempRegisterIndex++);

            IR_Sub64(Result, Left, Right);
        } break;

        case NodeKind_Mul:
        {
            ir_operand Left = GenerateNodeIR(Node->Binary.Left);
            ir_operand Right = GenerateNodeIR(Node->Binary.Right);

            Result = IR_TempOperand(TempRegisterIndex++);

            IR_MulI64(Result, Left, Right);
        } break;

        case NodeKind_Div:
        {
            ir_operand Left = GenerateNodeIR(Node->Binary.Left);
            ir_operand Right = GenerateNodeIR(Node->Binary.Right);

            Result = IR_TempOperand(TempRegisterIndex++);

            IR_DivI64(Result, Left, Right);
        } break;

        case NodeKind_Mod:
        {
            ir_operand Left = GenerateNodeIR(Node->Binary.Left);
            ir_operand Right = GenerateNodeIR(Node->Binary.Right);

            Result = IR_TempOperand(TempRegisterIndex++);

            IR_ModI64(Result, Left, Right);
        } break;
    }

    return (Result);
}

local ir_block GenerateIR(node_list List)
{
    ir_block Block =
    {
        .First = OperationCountIR,
        .Count = 0,
    };

    for (
        node_id Current = List.First;
        Current != NilNodeID;
    )
    {
        if (Current == List.Last)
            IR_Ret(GenerateNodeIR(Current));
        else
            GenerateNodeIR(Current);

        node* Node = GetNode(Current);
        Current = Node->Next;
    }

    Block.Count = OperationCountIR - Block.First;
    Block.TempRegisterCount = TempRegisterIndex;

    return (Block);
}

local void PrintOperandIR(ir_operand Operand)
{
    usize Written = 0;

    switch (Operand.Kind)
    {
        default: break;

        case IR_OperandKind_Imm:
            Written += PrintUSize(Operand.Imm.Value);
            break;

        case IR_OperandKind_Temp:
            Written += Print(Str("T"));
            Written += PrintUSize(Operand.Temp.Index);
            break;
    }

    if (Written)
        RightPadOutput(Written, 16);
}

local void PrintIR(ir_op_id OpID)
{
    persist string OpCodeNames[] =
    {
        #define DefineOpCodeName(Name) StaticStr(#Name),
        AllOpCodesIR(DefineOpCodeName)
        #undef DefineOpCodeName
    };

    ir_op* Op = OperationsIR + OpID;

    {
        usize Written = 0;
        Written += Print(Str("%"));
        Written += PrintUSize(OpID);
        RightPadOutput(Written, 6);
    }

    RightPadOutput(Print(OpCodeNames[Op->Code]), 16);

    PrintOperandIR(Op->Dest);
    PrintOperandIR(Op->Source1);
    PrintOperandIR(Op->Source2);
}

local void PrintBlockIR(ir_block Block)
{
    for (u32 Index = 0; Index < Block.Count; Index++)
    {
        PrintIR(Block.First + Index);
        PrintNewLine();
    }
}

