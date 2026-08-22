
// ==========================================================================================
// NOTE(vak): Main program logic
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
#include "print.c"
#include "lexer.c"
#include "parser.c"

// ==========================================================================================
// NOTE(vak): Main
// ==========================================================================================

local void Main(void)
{
    string Code = Str("120 / 2*(10 + 10)");

    token_array_id  TokenArrayID    = CreateTokenArray();
    parser_id       ParserID        = CreateParser();

    Tokenize(TokenArrayID, Code);

    node_id RootNode = Parse(ParserID, TokenArrayID);

    // NOTE(vak): Original code string
    {
        Print(StdOut, Str("Code = '"));
        Print(StdOut, Code);
        Print(StdOut, Str("'"));
        PrintNewLine(StdOut);
    }

    // NOTE(vak): Tokenizer output
    {
        Println(StdOut, Str("Tokenizer output:"));

        u32 TokenCount = GetTokenCount(TokenArrayID);
        for (token_id TokenID = 0; TokenID < TokenCount; TokenID++)
        {
            Print(StdOut, Str("    "));
            Print(StdOut, Str("TokenID = "));
            RightPadOutput(StdOut, PrintUSize(StdOut, TokenID), 16);
            Print(StdOut, Str("String = '"));
            Print(StdOut, GetTokenString(TokenArrayID, TokenID));
            Print(StdOut, Str("'"));
            PrintNewLine(StdOut);
        }
    }

    // NOTE(vak): Parser output
    {
        Println(StdOut, Str("Parser output:"));
        PrintNode(StdOut, ParserID, RootNode);
    }
}

