
// ==========================================================================================
// NOTE(vak): Expression parser (implements a subset of parser.c)
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local node_id ParseExpression(parser_id ParserID)
{
    node_id Node = ParseSum(ParserID);
    return (Node);
}

local node_id ParseSum(parser_id ParserID)
{
    parser* Parser = GetParser(ParserID);
    node_id Node = ParseFactor(ParserID);

    for (;;)
    {
        token_id TokenID = Parser->TokenID;

        if (ParserNextIfMatch(ParserID, '+'))
        {
            Node = PushBinaryNode(ParserID, NodeKind_Add, TokenID, Node, ParseFactor(ParserID));
        }
        else if (ParserNextIfMatch(ParserID, '-'))
        {
            Node = PushBinaryNode(ParserID, NodeKind_Sub, TokenID, Node, ParseFactor(ParserID));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node_id ParseFactor(parser_id ParserID)
{
    parser* Parser = GetParser(ParserID);
    node_id Node = ParsePrimary(ParserID);

    for (;;)
    {
        token_id TokenID = Parser->TokenID;

        if (ParserNextIfMatch(ParserID, '*'))
        {
            Node = PushBinaryNode(ParserID, NodeKind_Mul, TokenID, Node, ParsePrimary(ParserID));
        }
        else if (ParserNextIfMatch(ParserID, '/'))
        {
            Node = PushBinaryNode(ParserID, NodeKind_Div, TokenID, Node, ParsePrimary(ParserID));
        }
        else if (ParserNextIfMatch(ParserID, '%'))
        {
            Node = PushBinaryNode(ParserID, NodeKind_Mod, TokenID, Node, ParsePrimary(ParserID));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node_id ParsePrimary(parser_id ParserID)
{
    parser* Parser = GetParser(ParserID);
    node_id NodeID = 0;

    token_id TokenID = Parser->TokenID;

    if (ParserNextIfMatch(ParserID, TokenKind_Integer))
    {
        NodeID = PushIntegerNode(ParserID, TokenID);
    }
    else if (ParserNextIfMatch(ParserID, '('))
    {
        NodeID = ParseExpression(ParserID);
        ParserExpectAndSkip(ParserID, ')', Str("expected matching ')'"));
    }
    else
    {
        ParserError(ParserID, Str("syntax error"));
    }

    return (NodeID);
}

