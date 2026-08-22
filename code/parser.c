
// ==========================================================================================
// NOTE(vak): Compiler parser: responsible for converting a sequence of tokens into a
// syntax tree (it's more like a directed acyclic graph) composed of operations.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
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

typedef struct
{
    usize Value;
} integer_node;

typedef struct
{
    node_id Left;
    node_id Right;
} binary_node;

typedef struct
{
    u32 U32[1];
    u16 U16[2];
    u8  U8 [4];
} parser_id;

#define NilParserID (parser_id){0}
#define IsNilParserID(ParserID) ((ParserID).U32[0] == 0)

// NOTE(vak): Basic functionality (implemented in parser_basic.c)

local parser_id     CreateParser        (void);
local void          DestroyParser       (parser_id ParserID);
local node_id       Parse               (parser_id ParserID, token_array_id TokenArrayID);
local void          BeginParsing        (parser_id ParserID, token_array_id TokenArrayID);
local void          EndParsing          (parser_id ParserID);

// NOTE(vak): Expression parser (implemented in parser_expression.c)

local node_id       ParseExpression     (parser_id ParserID);
local node_id       ParseSum            (parser_id ParserID);
local node_id       ParseFactor         (parser_id ParserID);
local node_id       ParsePrimary        (parser_id ParserID);

// NOTE(vak): Utility functions (implemented in parser_utility.c)

local node_kind     GetNodeKind         (parser_id ParserID, node_id NodeID);
local integer_node  GetIntegerNode      (parser_id ParserID, node_id NodeID);
local binary_node   GetBinaryNode       (parser_id ParserID, node_id NodeID);
local usize         PrintNode           (print_out Out, parser_id ParserID, node_id NodeID);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

typedef struct
{
    node_kind       Kind;
    token_array_id  TokenArrayID;
    token_id        TokenID;
    union
    {
        integer_node    Integer;
        binary_node     Binary;
    };
} node;

typedef struct
{
    arena_id        NodeArenaID;    // NOTE(vak): Storage for nodes that was parsed by this parser
    token_array_id  TokenArrayID;   // NOTE(vak): Current token array that is being parsed
    token_id        TokenID;        // NOTE(vak): Current token that is being parsed
} parser;

#define DefaultNodeArenaCommited    (16384  * sizeof(node))
#define DefaultNodeArenaReserved    (U32Max * sizeof(node))

local parser Parsers[512] = {0};

#include "parser_helper.c"
#include "parser_basic.c"
#include "parser_expression.c"
#include "parser_utility.c"

