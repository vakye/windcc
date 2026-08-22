
// ==========================================================================================
// NOTE(vak): Utility functions of a parser (implements a subset of parser.c)
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local node_kind GetNodeKind(parser_id ParserID, node_id NodeID)
{
    node* Node = GetNode(ParserID, NodeID);
    node_kind Result = Node->Kind;
    return (Result);
}

local integer_node GetIntegerNode(parser_id ParserID, node_id NodeID)
{
    node* Node = GetNode(ParserID, NodeID);
    integer_node Result = Node->Integer;
    return (Result);
}

local binary_node GetBinaryNode(parser_id ParserID, node_id NodeID)
{
    node* Node = GetNode(ParserID, NodeID);
    binary_node Result = Node->Binary;
    return (Result);
}

local usize PrintNode(print_out Out, parser_id ParserID, node_id NodeID)
{
    persist usize Depth = 0;

    Depth++;

    usize Written = 0;

    for (usize Index = 0; Index < Depth; Index++)
        Written += Print(Out, Str("    "));

    node* Node = GetNode(ParserID, NodeID);

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
            integer_node Integer = GetIntegerNode(ParserID, NodeID);

            Written += PrintUSize(Out, Integer.Value);
            Written += PrintNewLine(Out);
        } break;

        case NodeKind_Add:
        case NodeKind_Sub:
        case NodeKind_Mul:
        case NodeKind_Div:
        case NodeKind_Mod:
        {
            binary_node Binary = GetBinaryNode(ParserID, NodeID);

            Written += PrintNewLine(Out);
            Written += PrintNode(Out, ParserID, Binary.Left);
            Written += PrintNode(Out, ParserID, Binary.Right);
        } break;
    }

    Depth--;

    return (Written);
}

