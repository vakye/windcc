
// ==========================================================================================
// NOTE(vak): Primitive functions of a parser (implements a subset of parser.c)
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local parser_id CreateParser(void)
{
    parser_id ParserID = FindFreeParserSlot();
    AlwaysAssert(!IsNilParserID(ParserID));

    parser* Parser = GetParser(ParserID);

    ZeroType(Parser);

    Parser->NodeArenaID = CreateArena(
        DefaultNodeArenaCommited,
        DefaultNodeArenaReserved
    );

    return (ParserID);
}

local void DestroyParser(parser_id ParserID)
{
    parser* Parser = GetParser(ParserID);
    DestroyArena(Parser->NodeArenaID);
    ZeroType(Parser);
}

local node_id Parse(parser_id ParserID, token_array_id TokenArrayID)
{
    BeginParsing(ParserID, TokenArrayID);
    node_id RootNodeID = ParseExpression(ParserID);
    EndParsing(ParserID);

    return (RootNodeID);
}

local void BeginParsing(parser_id ParserID, token_array_id TokenArrayID)
{
    parser* Parser = GetParser(ParserID);

    ResetArena(Parser->NodeArenaID);

    Parser->TokenArrayID    = TokenArrayID;
    Parser->TokenID         = 0;
}

local void EndParsing(parser_id ParserID)
{
    parser* Parser = GetParser(ParserID);

    Parser->TokenArrayID    = NilTokenArrayID;
    Parser->TokenID         = 0;
}

