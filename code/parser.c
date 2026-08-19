
#pragma once

#define AllPrimaryNodeKinds(X) \
    X(Integer) \
    X(Identifier)

#define AllPrefixNodeKinds(X) \
    X(Negate) \
    X(BitwiseNot) \
    X(LogicalNot) \
    X(AddressOf) \
    X(Dereference) \
    X(PreIncrement) \
    X(PreDecrement)

#define AllPostfixNodeKinds(X) \
    X(PostIncrement) \
    X(PostDecrement)

#define AllUnaryNodeKinds(X) \
    AllPrefixNodeKinds(X) \
    AllPostfixNodeKinds(X)

#define AllBinaryNodeKinds(X) \
    X(Add) \
    X(Sub) \
    X(Mul) \
    X(Div) \
    X(Mod) \
    \
    X(ShiftLeft) \
    X(ShiftRight) \
    \
    X(Equal) \
    X(NotEqual) \
    X(Less) \
    X(Greater) \
    X(LessEqual) \
    X(GreaterEqual) \
    \
    X(BitwiseAnd) \
    X(BitwiseXor) \
    X(BitwiseOr) \
    \
    X(LogicalAnd) \
    X(LogicalOr) \
    \
    X(Assign) \
    X(AssignAdd) \
    X(AssignSub) \
    X(AssignMul) \
    X(AssignDiv) \
    X(AssignMod) \
    X(AssignBitwiseAnd) \
    X(AssignBitwiseXor) \
    X(AssignBitwiseOr) \
    X(AssignShiftLeft) \
    X(AssignShiftRight)

#define AllStatementNodeKinds(X) \
    X(If) \
    X(For) \
    X(Break) \
    X(Continue) \
    X(Return)

#define AllNodeKinds(X) \
    X(Nil) \
    X(Ignore) \
    \
    AllPrimaryNodeKinds(X) \
    AllUnaryNodeKinds(X) \
    AllBinaryNodeKinds(X) \
    AllStatementNodeKinds(X) \
    \
    X(Ternary) \
    X(Block)

typedef enum
{
    #define DefineNodeKind(Name) NodeKind_##Name,
        AllNodeKinds(DefineNodeKind)
    #undef DefineNodeKind

    NodeKind_COUNT,
} node_kind;

typedef u32 node_id;

#define NilNodeID (0)

typedef struct
{
    node_id First;
    node_id Last;
} node_list;

#define NilNodeList (node_list){NilNodeID, NilNodeID}

typedef struct node node;
struct node
{
    node_kind Kind;
    node_id Next;
    token Token;

    union
    {
        struct { usize Value; } Integer;
        struct { string Value; } Identifier;

        struct
        {
            node_id Operand;
        } Unary;

        struct
        {
            node_id Left;
            node_id Right;
        } Binary;

        struct
        {
            node_id Left;
            node_id Middle;
            node_id Right;
        } Ternary;

        struct
        {
            node_id Condition;
            node_id Then;
            node_id Else;
        } If;

        struct
        {
            node_id Initializer;
            node_id Condition;
            node_id PostIteration;
            node_id Body;
        } For;

        struct
        {
            node_id Value;
        } Return;

        struct
        {
            node_list Statements;
        } Block;
    };
};

// TODO(vak): Will be replaced by an arena allocator eventually....
local node Nodes[4096] = {0};
local u32 NodeCount = 1; // NOTE(vak): First spot reserved for NilNodeID

local node* GetNode(node_id NodeID)
{
    persist node NilNode = {0};

    node* Result = &NilNode;

    if ((NodeID != NilNodeID) && (NodeID < ArrayCount(Nodes)))
        Result = Nodes + NodeID;
    else
        ZeroType(&NilNode);

    return (Result);
}

local node_id PushNode(node_kind Kind, token Token)
{
    if (NodeCount == ArrayCount(Nodes))
    {
        Println(Str("ERROR: ran out of node memory"));
        Exit(1);
    }

    node_id NodeID = NodeCount++;

    ZeroType(Nodes + NodeID);

    Nodes[NodeID].Kind = Kind;
    Nodes[NodeID].Token = Token;

    return (NodeID);
}

local node_id PushIntegerNode(token Token)
{
    node_id NodeID = PushNode(NodeKind_Integer, Token);

    Nodes[NodeID].Integer.Value = TokenToInteger(Token);

    return (NodeID);
}

local node_id PushIdentifierNode(token Token)
{
    node_id NodeID = PushNode(NodeKind_Identifier, Token);

    Nodes[NodeID].Identifier.Value = TokenToString(Token);

    return (NodeID);
}

local node_id PushUnaryNode(node_kind Kind, token Token, node_id Operand)
{
    node_id NodeID = PushNode(Kind, Token);

    Nodes[NodeID].Unary.Operand = Operand;

    return (NodeID);
}

local node_id PushBinaryNode(node_kind Kind, token Token, node_id Left, node_id Right)
{
    node_id NodeID = PushNode(Kind, Token);

    Nodes[NodeID].Binary.Left = Left;
    Nodes[NodeID].Binary.Right = Right;

    return (NodeID);
}

local node_id PushTernaryNode(node_kind Kind, token Token, node_id Left, node_id Middle, node_id Right)
{
    node_id NodeID = PushNode(Kind, Token);

    Nodes[NodeID].Ternary.Left = Left;
    Nodes[NodeID].Ternary.Middle = Middle;
    Nodes[NodeID].Ternary.Right = Right;

    return (NodeID);
}

local node_id PushIfNode(token Token, node_id Condition, node_id Then, node_id Else)
{
    node_id NodeID = PushNode(NodeKind_If, Token);

    Nodes[NodeID].If.Condition  = Condition;
    Nodes[NodeID].If.Then       = Then;
    Nodes[NodeID].If.Else       = Else;

    return (NodeID);
}

local node_id PushForNode(
    token Token,
    node_id Initializer, 
    node_id Condition,
    node_id PostIteration,
    node_id Body
)
{
    node_id NodeID = PushNode(NodeKind_For, Token);

    Nodes[NodeID].For.Initializer   = Initializer;
    Nodes[NodeID].For.Condition     = Condition;
    Nodes[NodeID].For.PostIteration = PostIteration;
    Nodes[NodeID].For.Body          = Body;

    return (NodeID);
}

local node_id PushBlockNode(
    token Token,
    node_list Statements
)
{
    node_id NodeID = PushNode(NodeKind_Block, Token);

    Nodes[NodeID].Block.Statements = Statements;

    return (NodeID);
}

local node_id PushReturnNode(
    token Token,
    node_id Value
)
{
    node_id NodeID = PushNode(NodeKind_Return, Token);

    Nodes[NodeID].Return.Value = Value;

    return (NodeID);
}


local void AppendNodeList(node_list* List, node_id NodeID)
{
    if (NodeID == NilNodeID)
        return;

    if (List->First == NilNodeID)
    {
        List->First = NodeID;
        List->Last = NodeID;
    }
    else
    {
        node* Last = GetNode(List->Last);
        Last->Next = NodeID;
        List->Last = NodeID;
    }
}

local node_id ParseExpression(void);
local node_id ParseStatement(void);

local void ParserExpect(token_kind Kind, string Message)
{
    if (!MatchToken(Kind))
    {
        Print(Str("ERROR: "));
        Println(Message);
        Exit(1);
    }
}

local void ParserExpectAndSkip(token_kind Kind, string Message)
{
    if (!NextIfMatchToken(Kind))
    {
        Print(Str("ERROR: "));
        Println(Message);
        Exit(1);
    }
}

typedef enum
{
    // NOTE(vak): Reference:
    // https://en.cppreference.com/c/language/operator_precedence

    Precedence_Lowest = 0,

    Precedence_Postfix,
    Precedence_Prefix,
    Precedence_Factor,
    Precedence_Sum,
    Precedence_Shift,
    Precedence_Comparison,
    Precedence_Equality,
    Precedence_BitwiseAnd,
    Precedence_BitwiseXor,
    Precedence_BitwiseOr,
    Precedence_LogicalAnd,
    Precedence_LogicalOr,
    Precedence_Ternary,
    Precedence_Assign,

    Precedence_Highest,
} precedence;

typedef enum
{
    Associativity_LeftToRight = 0,
    Associativity_RightToLeft,
} associativity;

// NOTE(vak):
// Bit 0 - 6:   Precedennce (see parse_precedence)
// Bit 7:       Associativity (0 for left-to-right, 1 for right-to-left)

typedef u8 parse_op_info;

#define ParseOpInfo(Precedence, Associativity) ((Precedence) | ((Associativity) << 7))

#define ExtractPrecedence(ParseOpInfo) ((ParseOpInfo) & 0x7F)
#define ExtractAssociativity(ParseOpInfo) ((ParseOpInfo) >> 7)

typedef struct
{
    node_kind           Kind;
    parse_op_info       Info;
} parse_op;

persist parse_op HeadParseOps[] =
{
    ['+']                       = { NodeKind_Ignore,        ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    ['-']                       = { NodeKind_Negate,        ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    ['~']                       = { NodeKind_BitwiseNot,    ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    ['!']                       = { NodeKind_LogicalNot,    ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    ['&']                       = { NodeKind_AddressOf,     ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    ['*']                       = { NodeKind_Dereference,   ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    [TokenKind_DoublePlus]      = { NodeKind_PreIncrement,  ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
    [TokenKind_DoubleMinus]     = { NodeKind_PreDecrement,  ParseOpInfo(Precedence_Prefix, Associativity_RightToLeft) },
};

persist parse_op TailParseOps[] =
{
    ['+']                               = { NodeKind_Add,                   ParseOpInfo(Precedence_Sum,        Associativity_LeftToRight) },
    ['-']                               = { NodeKind_Sub,                   ParseOpInfo(Precedence_Sum,        Associativity_LeftToRight) },
    ['*']                               = { NodeKind_Mul,                   ParseOpInfo(Precedence_Factor,     Associativity_LeftToRight) },
    ['/']                               = { NodeKind_Div,                   ParseOpInfo(Precedence_Factor,     Associativity_LeftToRight) },
    ['%']                               = { NodeKind_Mod,                   ParseOpInfo(Precedence_Factor,     Associativity_LeftToRight) },
    [TokenKind_DoubleLess]              = { NodeKind_ShiftLeft,             ParseOpInfo(Precedence_Shift,      Associativity_LeftToRight) },
    [TokenKind_DoubleGreater]           = { NodeKind_ShiftRight,            ParseOpInfo(Precedence_Shift,      Associativity_LeftToRight) },
    [TokenKind_DoubleEqual]             = { NodeKind_Equal,                 ParseOpInfo(Precedence_Equality,   Associativity_LeftToRight) },
    [TokenKind_BangEqual]               = { NodeKind_NotEqual,              ParseOpInfo(Precedence_Equality,   Associativity_LeftToRight) },
    ['<']                               = { NodeKind_Less,                  ParseOpInfo(Precedence_Comparison, Associativity_LeftToRight) },
    ['>']                               = { NodeKind_Greater,               ParseOpInfo(Precedence_Comparison, Associativity_LeftToRight) },
    [TokenKind_LessEqual]               = { NodeKind_LessEqual,             ParseOpInfo(Precedence_Comparison, Associativity_LeftToRight) },
    [TokenKind_GreaterEqual]            = { NodeKind_GreaterEqual,          ParseOpInfo(Precedence_Comparison, Associativity_LeftToRight) },
    ['&']                               = { NodeKind_BitwiseAnd,            ParseOpInfo(Precedence_BitwiseAnd, Associativity_LeftToRight) },
    ['^']                               = { NodeKind_BitwiseXor,            ParseOpInfo(Precedence_BitwiseXor, Associativity_LeftToRight) },
    ['|']                               = { NodeKind_BitwiseOr,             ParseOpInfo(Precedence_BitwiseOr,  Associativity_LeftToRight) },
    [TokenKind_DoubleAmpersand]         = { NodeKind_LogicalAnd,            ParseOpInfo(Precedence_LogicalAnd, Associativity_LeftToRight) },
    [TokenKind_DoubleBar]               = { NodeKind_LogicalOr,             ParseOpInfo(Precedence_LogicalOr,  Associativity_LeftToRight) },

    ['=']                               = { NodeKind_Assign,                ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_PlusEqual]               = { NodeKind_AssignAdd,             ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_MinusEqual]              = { NodeKind_AssignSub,             ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_StarEqual]               = { NodeKind_AssignMul,             ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_SlashEqual]              = { NodeKind_AssignDiv,             ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_PercentEqual]            = { NodeKind_AssignMod,             ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_AmpersandEqual]          = { NodeKind_AssignBitwiseAnd,      ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_HatEqual]                = { NodeKind_AssignBitwiseXor,      ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_BarEqual]                = { NodeKind_AssignBitwiseOr,       ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_DoubleLessEqual]         = { NodeKind_AssignShiftLeft,       ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },
    [TokenKind_DoubleGreaterEqual]      = { NodeKind_AssignShiftRight,      ParseOpInfo(Precedence_Assign,     Associativity_RightToLeft) },

    [TokenKind_DoublePlus]              = { NodeKind_PostIncrement,         ParseOpInfo(Precedence_Postfix,    Associativity_LeftToRight) },
    [TokenKind_DoubleMinus]             = { NodeKind_PostDecrement,         ParseOpInfo(Precedence_Postfix,    Associativity_LeftToRight) },

    ['?']                               = { NodeKind_Ternary,               ParseOpInfo(Precedence_Ternary,    Associativity_RightToLeft) },
};

local node_id ParseExpressionMain(precedence MinPrecedence);

#define ParseExpression() ParseExpressionMain(Precedence_Highest)

local node_id ParseExpressionHead(void)
{
    node_id Result = NilNodeID;

    token Token = GetCurrentToken();
    parse_op HeadOp = HeadParseOps[Token.Kind];

    if (HeadOp.Kind != NodeKind_Nil)
    {
        NextToken();

        precedence Precedence = ExtractPrecedence(HeadOp.Info);

        node_id Operand = ParseExpressionMain(Precedence);

        if (HeadOp.Kind != NodeKind_Ignore)
            Result = PushUnaryNode(HeadOp.Kind, Token, Operand);
        else
            Result = Operand;
    }
    else
    { 
        if (NextIfMatchToken(TokenKind_Integer))
        {
            Result = PushIntegerNode(Token);
        }
        else if (NextIfMatchToken(TokenKind_Identifier))
        {
            Result = PushIdentifierNode(Token);
        }
        else if (NextIfMatchToken('('))
        {
            Result = ParseExpression();
            ParserExpectAndSkip(')', Str("expected matching ')' in expression"));
        }
        else if (MatchToken(';'))
        {
        }
        else
        {
            Println(Str("ERROR: syntax error"));
            Exit(1);
        }
    }

    return (Result);
}

local b32 IsPostfixNodeKind(node_kind Kind)
{
    b32 Result = false;

    #define MatchNodeKind(Name) Result |= ((Kind) == (NodeKind_##Name));
        AllPostfixNodeKinds(MatchNodeKind)
    #undef MatchNodeKind

    return (Result);
}

local b32 IsBinaryNodeKind(node_kind Kind)
{
    b32 Result = false;

    #define MatchNodeKind(Name) Result |= ((Kind) == (NodeKind_##Name));
        AllBinaryNodeKinds(MatchNodeKind)
    #undef MatchNodeKind

    return (Result);
}

local node_id ParseExpressionTail(precedence MinPrecedence, node_id Left)
{
    node_id Result = Left;

    token Token = GetCurrentToken();
    parse_op TailOp = TailParseOps[Token.Kind];

    while (TailOp.Kind != NodeKind_Nil)
    {
        precedence    Precedence    = ExtractPrecedence(TailOp.Info);
        associativity Associativity = ExtractAssociativity(TailOp.Info);

        if (Precedence > MinPrecedence)
            break;

        if (Associativity == Associativity_LeftToRight)
            if (Precedence == MinPrecedence)
                break;

        NextToken();

        if (IsBinaryNodeKind(TailOp.Kind))
        {
            node_id Right = ParseExpressionMain(Precedence);
            Result = PushBinaryNode(TailOp.Kind, Token, Result, Right);
        }
        else if (IsPostfixNodeKind(TailOp.Kind))
        {
            Result = PushUnaryNode(TailOp.Kind, Token, Result);
        }
        else if (TailOp.Kind == NodeKind_Ternary)
        {
            node_id Left   = Result;
            node_id Middle = ParseExpressionMain(Precedence_Ternary);

            ParserExpectAndSkip(':', Str("missing ':' in ternary expression"));

            node_id Right = ParseExpressionMain(Precedence_Ternary);

            Result = PushTernaryNode(NodeKind_Ternary, Token, Left, Middle, Right);
        }

        Token = GetCurrentToken();
        TailOp = TailParseOps[Token.Kind];
    }

    return (Result);
}

local node_id ParseExpressionMain(precedence MinPrecedence)
{
    node_id Result = NilNodeID;

    Result = ParseExpressionHead();
    Result = ParseExpressionTail(MinPrecedence, Result);

    return (Result);
}

local node_id ParseBlock(void)
{
    token FirstToken = GetCurrentToken();

    ParserExpectAndSkip('{', Str("expected '{' at start of block"));

    node_list Statements = NilNodeList;

    for (;;)
    {
        if (MatchToken(TokenKind_EOF))  break;
        if (MatchToken('}'))            break;

        AppendNodeList(&Statements, ParseStatement());
    }

    ParserExpectAndSkip('}', Str("expected '}' at end of block"));

    node_id Result = PushBlockNode(FirstToken, Statements);
    return (Result);
}

local node_id ParseStatement(void)
{
    node_id Result = NilNodeID;

    token FirstToken = GetCurrentToken();

    if (NextIfMatchToken(TokenKind_If))
    {
        ParserExpect('(', Str("expected '(' after 'if'"));

        node_id Condition = ParseExpression();
        node_id Then      = ParseStatement();
        node_id Else      = NilNodeID;

        if (NextIfMatchToken(TokenKind_Else))
            Else = ParseStatement();

        Result = PushIfNode(FirstToken, Condition, Then, Else);
    }
    else if (NextIfMatchToken(TokenKind_For))
    {
        ParserExpectAndSkip('(', Str("ERROR: expected '(' after 'for'"));

        node_id Initializer     = NilNodeID;
        node_id Condition       = NilNodeID;
        node_id PostIteration   = NilNodeID;
        node_id Body            = NilNodeID;

        Initializer = ParseExpression();

        ParserExpectAndSkip(';', Str("expected ';' after for-loop initializer"));

        Condition = ParseExpression();

        ParserExpectAndSkip(';', Str("expected ';' after for-loop condition"));

        PostIteration = ParseExpression();

        ParserExpectAndSkip(')', Str("expected ')' after for-loop post-iteration"));

        Body = ParseStatement();

        Result = PushForNode(FirstToken, Initializer, Condition, PostIteration, Body);
    }
    else if (NextIfMatchToken(TokenKind_While))
    {
        ParserExpectAndSkip('(', Str("expected '(' after 'while'"));

        node_id Initializer     = NilNodeID;
        node_id Condition       = NilNodeID;
        node_id PostIteration   = NilNodeID;
        node_id Body            = NilNodeID;

        Condition = ParseExpression();

        ParserExpectAndSkip(')', Str("expected ')' after while-loop condition"));

        Body = ParseStatement();

        Result = PushForNode(FirstToken, Initializer, Condition, PostIteration, Body);
    }
    else if (NextIfMatchToken(TokenKind_Do))
    {
        node_id Initializer     = NilNodeID;
        node_id Condition       = NilNodeID;
        node_id PostIteration   = NilNodeID;
        node_id Body            = NilNodeID;

        Body = ParseExpression();
        Initializer = Body; // TODO(vak): Get rid of this cheap hack!

        ParserExpectAndSkip(TokenKind_While, Str("expected 'while' after body in do-while loop"));
        ParserExpectAndSkip('(', Str("expected '(' after 'while'"));

        Condition = ParseExpression();

        ParserExpectAndSkip(')', Str("expected ')' after while-loop condition"));

        Result = PushForNode(FirstToken, Initializer, Condition, PostIteration, Body);
    }
    else if (NextIfMatchToken(TokenKind_Break))
    {
        Result = PushNode(NodeKind_Break, FirstToken);
    }
    else if (NextIfMatchToken(TokenKind_Continue))
    {
        Result = PushNode(NodeKind_Continue, FirstToken);
    }
    else if (NextIfMatchToken(TokenKind_Return))
    {
        node_id Value = ParseExpression();

        ParserExpectAndSkip(';', Str("expected ';' at end of statement"));

        Result = PushReturnNode(FirstToken, Value);
    }
    else
    {
        Result = ParseExpression();

        ParserExpectAndSkip(';', Str("expected ';' at end of statement"));
    }

    return (Result);
}

local node_list Parse(void)
{
    node_list List = NilNodeList;

    while (!MatchToken(TokenKind_EOF))
        AppendNodeList(&List, ParseStatement());

    return (List);
}

local void PrintNodeList(node_list List);

local void PrintNode(node_id NodeID)
{
    persist usize Depth = 0;

    Depth++;

    persist string NodeKindNames[] =
    {
        #define DefineNodeKindName(Name) StaticStr(#Name),
            AllNodeKinds(DefineNodeKindName)
        #undef DefineNodeKindName
    };

    node* Node = GetNode(NodeID);

    for (usize Index = 1; Index < Depth; Index++)
        Print(Str("    "));

    Print(NodeKindNames[Node->Kind]);
    Print(Str(": "));

    switch (Node->Kind)
    {
        default: PrintNewLine(); break;

        case NodeKind_Integer:
        {
            PrintUSize(Node->Integer.Value);
            PrintNewLine();
        } break;

        case NodeKind_Identifier:
        {
            Print(Node->Identifier.Value);
            PrintNewLine();
        } break;

        #define DefineNodeKindCase(Name) case NodeKind_##Name:

        AllUnaryNodeKinds(DefineNodeKindCase)
        {
            PrintNewLine();
            PrintNode(Node->Unary.Operand);
        } break;

        AllBinaryNodeKinds(DefineNodeKindCase)
        {
            PrintNewLine();
            PrintNode(Node->Binary.Left);
            PrintNode(Node->Binary.Right);
        } break;

        #undef DefineNodeKindCase

        case NodeKind_If:
        {
            PrintNewLine();
            PrintNode(Node->If.Condition);
            PrintNode(Node->If.Then);
            PrintNode(Node->If.Else);
        } break;

        case NodeKind_For:
        {
            PrintNewLine();
            PrintNode(Node->For.Initializer);
            PrintNode(Node->For.Condition);
            PrintNode(Node->For.PostIteration);
            PrintNode(Node->For.Body);
        } break;

        case NodeKind_Return:
        {
            PrintNewLine();
            PrintNode(Node->Return.Value);
        } break;

        case NodeKind_Ternary:
        {
            PrintNewLine();
            PrintNode(Node->Ternary.Left);
            PrintNode(Node->Ternary.Middle);
            PrintNode(Node->Ternary.Right);
        } break;

        case NodeKind_Block:
        {
            PrintNewLine();
            PrintNodeList(Node->Block.Statements);
        } break;
    }

    Depth--;
}

local void PrintNodeList(node_list List)
{
    for (
        node_id Current = List.First;
        Current != NilNodeID;
    )
    {
        PrintNode(Current);

        node* Node = GetNode(Current);
        Current = Node->Next;
    }
}

