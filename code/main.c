
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

    SetupLexer();
    SetupParser();

    token_array Tokens = Tokenize(Code);
    node_id Node = Parse(Tokens);

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

        for (u32 Offset = 0; Offset < Tokens.Count; Offset ++)
        {
            token_id TokenID = Tokens.First + Offset;

            Print(StdOut, Str("    "));
            Print(StdOut, Str("TokenID = "));
            RightPadOutput(StdOut, PrintUSize(StdOut, TokenID), 16);
            Print(StdOut, Str("String = '"));
            Print(StdOut, GetTokenString(TokenID));
            Print(StdOut, Str("'"));
            PrintNewLine(StdOut);
        }
    }

    // NOTE(vak): Parser output
    {
        Println(StdOut, Str("Parser output:"));
        PrintNode(StdOut, Node);
    }
}

