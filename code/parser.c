
// ==========================================================================================
// NOTE(vak): Compiler parser: Converts a sequence of tokens into a syntax tree composed of
// operations.
// ==========================================================================================

// NOTE(vak): Interface
#pragma once

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef enum
{
    NodeKind_Nil = 0,

    NodeKind_Integer,
    NodeKind_Add,
} node_kind;

typedef struct node node;
struct node
{
    node_kind Kind;
    token Token;
    usize Integer;
    node* Left;
    node* Right;
};

local node* Parse(void);
local node* ParseExpression(void);
local node* ParseSum(void);
local node* ParsePrimary(void);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local arena_id NodeArenaID = NilArenaID;

local node* PushNode(node_kind Kind, token Token)
{
    if (IsNilArenaID(NodeArenaID))
        NodeArenaID = CreateArena(MB(1), GB(16));

    node* Node = PushArena(NodeArenaID, node);

    ZeroType(Node);
    Node->Kind = Kind;
    Node->Token = Token;

    return (Node);
}

local node* PushIntegerNode(token Token)
{
    node* Node = PushNode(NodeKind_Integer, Token);
    Node->Integer = TokenToInteger(Token);
    return (Node);
}

local node* PushBinaryNode(node_kind Kind, token Token, node* Left, node* Right)
{
    node* Node = PushNode(NodeKind_Integer, Token);
    Node->Left = Left;
    Node->Right = Right;

    return (Node);
}

local void ParserExpect(token_kind Kind, string ErrorMessage)
{
    if (!MatchToken(Kind))
    {
        Print(Str("ERROR: "));
        Println(ErrorMessage);
        Exit(1);
    }
}

local void ParserExpectAndSkip(token_kind Kind, string ErrorMessage)
{
    if (!NextIfMatchToken(Kind))
    {
        Print(Str("ERROR: "));
        Println(ErrorMessage);
        Exit(1);
    }
}

local node* Parse(void)
{
    node* Node = ParseExpression();
    return (Node);
}

local node* ParseExpression(void)
{
    node* Node = ParseSum();
    return (Node);
}

local node* ParseSum(void)
{
    node* Node = ParsePrimary();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('+'))
        {
            Node = PushBinaryNode(NodeKind_Add, Token, Node, ParsePrimary());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParsePrimary(void)
{
    node* Node = 0;

    token Token = GetCurrentToken();
    if (NextIfMatchToken(TokenKind_Integer))
    {
        Node = PushIntegerNode(Token);
    }
    else if (NextIfMatchToken('('))
    {
        Node = ParseExpression();
        ParserExpectAndSkip(')', Str("expected matching ')'"));
    }
    else
    {
        Println(Str("ERROR: syntax error"));
        Exit(1);
    }

    return (Node);
}

