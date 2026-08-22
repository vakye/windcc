
// ==========================================================================================
// NOTE(vak): Compiler parser: responsible for converting a sequence of tokens into a
// syntax tree (it's more like a directed acyclic graph) composed of operations.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
#include "memory.c"
#include "print.c"
#include "lexer.c"

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef enum
{
    NodeKind_Nil = 0,

    NodeKind_Integer,       // NOTE(vak): Uses integer_node

    // NOTE(vak): Binary nodes (Uses binary_node)

    NodeKind_Add,
    NodeKind_Sub,
    NodeKind_Mul,
    NodeKind_Div,
    NodeKind_Mod,

    NodeKind_COUNT,
} node_kind;

typedef u32 node_id;

#define NilNodeID (0)

typedef struct
{
    usize Value;
} integer_node;

typedef struct
{
    node_id Left;
    node_id Right;
} binary_node;

local void          SetupParser         (void);
local void          ShutdownParser      (void);
local node_id       Parse               (token_array Tokens);

local node_kind     GetNodeKind         (node_id NodeID);
local integer_node  GetIntegerNode      (node_id NodeID);
local binary_node   GetBinaryNode       (node_id NodeID);
local usize         PrintNode           (print_out Out, node_id NodeID);

local void          SetParserTarget     (token_array Tokens);
local node_id       ParseExpression     (void);
local node_id       ParseSum            (void);
local node_id       ParseFactor         (void);
local node_id       ParsePrimary        (void);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

typedef struct
{
    node_kind       Kind;
    token_id        TokenID;
    union
    {
        integer_node    Integer;
        binary_node     Binary;
    };
} node;

#define DefaultNodeArenaCommited    (16384  * sizeof(node))
#define DefaultNodeArenaReserved    (U32Max * sizeof(node))

local arena_id      NodeArenaID     = NilArenaID;
local token_array   ParsingTokens   = {0};

local void SetupParser(void)
{
    NodeArenaID = CreateArena(
        DefaultNodeArenaCommited,
        DefaultNodeArenaReserved
    );
}

local void ShutdownParser(void)
{
    DestroyArena(NodeArenaID);
    NodeArenaID = NilArenaID;
}

local node_id Parse(token_array Tokens)
{
    SetParserTarget(Tokens);
    node_id Result = ParseExpression();
    return (Result);
}

local node* GetNode(node_id NodeID)
{
    persist node NilNode = {0};

    usize NodeCount = (GetArenaUsed(NodeArenaID) / sizeof(node));

    node* Result = &NilNode;

    if ((NodeID > 0) && (NodeID <= NodeCount))
        Result = (node*)GetArenaBase(NodeArenaID) + (NodeID - 1);

    return (Result);
}

local node_kind GetNodeKind(node_id NodeID)
{
    node* Node = GetNode(NodeID);
    node_kind Result = Node->Kind;
    return (Result);
}

local integer_node GetIntegerNode(node_id NodeID)
{
    node* Node = GetNode(NodeID);
    integer_node Result = Node->Integer;
    return (Result);
}

local binary_node GetBinaryNode(node_id NodeID)
{
    node* Node = GetNode(NodeID);
    binary_node Result = Node->Binary;
    return (Result);
}

local usize PrintNode(print_out Out, node_id NodeID)
{
    persist usize Depth = 0;

    Depth++;

    usize Written = 0;

    for (usize Index = 0; Index < Depth; Index++)
        Written += Print(Out, Str("    "));

    node* Node = GetNode(NodeID);

    persist string NodeKindStrings[NodeKind_COUNT] =
    {
        [NodeKind_Nil]          = StaticStr("Nil"),
        [NodeKind_Integer]      = StaticStr("Integer"),
        [NodeKind_Add]          = StaticStr("Add"),
        [NodeKind_Sub]          = StaticStr("Sub"),
        [NodeKind_Mul]          = StaticStr("Mul"),
        [NodeKind_Div]          = StaticStr("Div"),
        [NodeKind_Mod]          = StaticStr("Mod"),
    };

    Written += Print(Out, NodeKindStrings[Node->Kind]);
    Written += Print(Out, Str(": "));

    switch (Node->Kind)
    {
        default: {} break;

        case NodeKind_Integer:
        {
            integer_node Integer = GetIntegerNode(NodeID);

            Written += PrintUSize(Out, Integer.Value);
            Written += PrintNewLine(Out);
        } break;

        case NodeKind_Add:
        case NodeKind_Sub:
        case NodeKind_Mul:
        case NodeKind_Div:
        case NodeKind_Mod:
        {
            binary_node Binary = GetBinaryNode(NodeID);

            Written += PrintNewLine(Out);
            Written += PrintNode(Out, Binary.Left);
            Written += PrintNode(Out, Binary.Right);
        } break;
    }

    Depth--;

    return (Written);
}

local node_id PushNode(node_kind Kind, token_id TokenID)
{
    node_id NodeID = 1 + (node_id)(GetArenaUsed(NodeArenaID) / sizeof(node));
    PushArena(NodeArenaID, node);

    node* Node = GetNode(NodeID);
    ZeroType(Node);
    Node->Kind = Kind;
    Node->TokenID = TokenID;

    return (NodeID);
}

local node_id PushIntegerNode(token_id TokenID)
{
    node_id NodeID = PushNode(NodeKind_Integer, TokenID);
    node* Node = GetNode(NodeID);

    Node->Integer.Value = GetTokenInteger(TokenID);

    return (NodeID);
}

local node_id PushBinaryNode(node_kind Kind, token_id TokenID, node_id Left, node_id Right)
{
    node_id NodeID = PushNode(Kind, TokenID);
    node* Node = GetNode(NodeID);

    Node->Binary.Left = Left;
    Node->Binary.Right = Right;

    return (NodeID);
}

local void SetParserTarget(token_array Tokens)
{
    ParsingTokens = Tokens;
}

local token_id ParserCurrent(void)
{
    token_id TokenID = NilTokenID;

    if (ParsingTokens.Count)
        TokenID = ParsingTokens.First;

    return (TokenID);
}

local void ParserNext(void)
{
    if (ParsingTokens.Count)
    {
        ParsingTokens.First++;
        ParsingTokens.Count--;
    }
}

local b32 ParserMatch(token_kind Kind)
{
    b32 Result = (GetTokenKind(ParserCurrent()) == Kind);
    return (Result);
}

local b32 ParserNextIfMatch(token_kind Kind)
{
    b32 Result = ParserMatch(Kind);
    if (Result)
        ParserNext();

    return (Result);
}

local void ParserError(string ErrorMessage)
{
    Print(StdErr, Str("ERROR: "));
    Println(StdErr, ErrorMessage);
    Exit(1);
}

local void ParserExpect(token_kind Kind, string ErrorMessage)
{
    if (!ParserMatch(Kind))
        ParserError(ErrorMessage);
}

local void ParserExpectAndSkip(token_kind Kind, string ErrorMessage)
{
    if (!ParserNextIfMatch(Kind))
        ParserError(ErrorMessage);
}

local node_id ParseExpression(void)
{
    node_id Node = ParseSum();
    return (Node);
}

local node_id ParseSum(void)
{
    node_id Node = ParseFactor();

    for (;;)
    {
        token_id TokenID = ParserCurrent();

        if (ParserNextIfMatch('+'))
        {
            Node = PushBinaryNode(NodeKind_Add, TokenID, Node, ParseFactor());
        }
        else if (ParserNextIfMatch('-'))
        {
            Node = PushBinaryNode(NodeKind_Sub, TokenID, Node, ParseFactor());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node_id ParseFactor(void)
{
    node_id Node = ParsePrimary();

    for (;;)
    {
        token_id TokenID = ParserCurrent();

        if (ParserNextIfMatch('*'))
        {
            Node = PushBinaryNode(NodeKind_Mul, TokenID, Node, ParsePrimary());
        }
        else if (ParserNextIfMatch('/'))
        {
            Node = PushBinaryNode(NodeKind_Div, TokenID, Node, ParsePrimary());
        }
        else if (ParserNextIfMatch('%'))
        {
            Node = PushBinaryNode(NodeKind_Mod, TokenID, Node, ParsePrimary());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node_id ParsePrimary(void)
{
    node_id NodeID = NilNodeID;
    token_id TokenID = ParserCurrent();

    if (ParserNextIfMatch(TokenKind_Integer))
    {
        NodeID = PushIntegerNode(TokenID);
    }
    else if (ParserNextIfMatch('('))
    {
        NodeID = ParseExpression();
        ParserExpectAndSkip(')', Str("expected matching ')'"));
    }
    else
    {
        ParserError(Str("syntax error"));
    }

    return (NodeID);
}

