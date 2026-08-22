
// ==========================================================================================
// NOTE(vak): Common internal helper functions used by parser.c implementation
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

local parser*       GetParser           (parser_id ParserID);
local parser_id     FindFreeParserSlot  (void);

local b32           ParserMatch         (parser_id ParserID, token_kind MatchKind);
local b32           ParserNextIfMatch   (parser_id ParserID, token_kind MatchKind);

local void          ParserError         (parser_id ParserID, string ErrorMessage);
local void          ParserExpect        (parser_id ParserID, token_kind MatchKind, string ErrorMessage);
local void          ParserExpectAndSkip (parser_id ParserID, token_kind MatchKind, string ErrorMessage);

local node*         GetNode             (parser_id ParserID, node_id NodeID);

local node_id       PushNode            (parser_id ParserID, node_kind Kind, token_id TokenID);
local node_id       PushIntegerNode     (parser_id ParserID, token_id TokenID);
local node_id       PushBinaryNode      (parser_id ParserID, node_kind NodeKind, token_id TokenID, node_id Left, node_id Right);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local parser* GetParser(parser_id ParserID)
{
    AlwaysAssert(ParserID.U32[0] > 0);
    AlwaysAssert(ParserID.U32[0] <= ArrayCount(Parsers));

    parser* Parser = Parsers + (ParserID.U32[0] - 1);
    return (Parser);
}

local parser_id FindFreeParserSlot(void)
{
    parser_id ParserID = NilParserID;

    for (u32 Index = 0; Index < ArrayCount(Parsers); Index++)
    {
        parser* Parser = Parsers + Index;
        if (IsNilArenaID(Parser->NodeArenaID))
        {
            ParserID.U32[0] = 1 + Index;
            break;
        }
    }

    return (ParserID);
}

local b32 ParserMatch(parser_id ParserID, token_kind MatchKind)
{
    parser* Parser = GetParser(ParserID);
    token_kind Kind = GetTokenKind(Parser->TokenArrayID, Parser->TokenID);
    b32 Result = (Kind == MatchKind);
    return (Result);
}

local b32 ParserNextIfMatch(parser_id ParserID, token_kind MatchKind)
{
    parser* Parser = GetParser(ParserID);

    b32 Result = ParserMatch(ParserID, MatchKind);
    if (Result)
        Parser->TokenID++;

    return (Result);
}

local void ParserError(parser_id ParserID, string ErrorMessage)
{
    Print(StdErr, Str("ERROR: "));
    Println(StdErr, ErrorMessage);
    Exit(1);
}

local void ParserExpect(parser_id ParserID, token_kind MatchKind, string ErrorMessage)
{
    if (!ParserMatch(ParserID, MatchKind))
        ParserError(ParserID, ErrorMessage);
}

local void ParserExpectAndSkip(parser_id ParserID, token_kind MatchKind, string ErrorMessage)
{
    if (!ParserNextIfMatch(ParserID, MatchKind))
        ParserError(ParserID, ErrorMessage);
}

local node* GetNode(parser_id ParserID, node_id NodeID)
{
    parser* Parser = GetParser(ParserID);
    usize Offset = NodeID * sizeof(node);

    AlwaysAssert(Offset < GetArenaUsed(Parser->NodeArenaID))

    node* Node = (node*)GetArenaBase(Parser->NodeArenaID) + NodeID;
    return (Node);
}

local node_id PushNode(parser_id ParserID, node_kind Kind, token_id TokenID)
{
    parser* Parser = GetParser(ParserID);

    node_id NodeID = GetArenaUsed(Parser->NodeArenaID) / sizeof(node);
    PushArena(Parser->NodeArenaID, node);

    node* Node = GetNode(ParserID, NodeID);
    ZeroType(Node);

    Node->Kind          = Kind;
    Node->TokenArrayID  = Parser->TokenArrayID;
    Node->TokenID       = TokenID;

    return (NodeID);
}

local node_id PushIntegerNode(parser_id ParserID, token_id TokenID)
{
    parser* Parser = GetParser(ParserID);

    node_id NodeID = PushNode(ParserID, NodeKind_Integer, TokenID);
    node* Node = GetNode(ParserID, NodeID);

    Node->Integer.Value = GetTokenInteger(Parser->TokenArrayID, TokenID);

    return (NodeID);
}

local node_id PushBinaryNode(
    parser_id ParserID,
    node_kind NodeKind,
    token_id TokenID,
    node_id Left,
    node_id Right
)
{
    parser* Parser = GetParser(ParserID);

    node_id NodeID = PushNode(ParserID, NodeKind, TokenID);
    node* Node = GetNode(ParserID, NodeID);

    Node->Binary.Left  = Left;
    Node->Binary.Right = Right;

    return (NodeID);
}

