
// ==========================================================================================
// NOTE(vak): Main program logic
// ==========================================================================================

#pragma once

#include "print.c"
#include "memory.c"
#include "character.c"
#include "lexer.c"

local void Main(void)
{
    string Code = Str("int main () { __hello + world_23_() * (10 + 10) / 120; }");

    token_array_id TokenArrayID = CreateTokenArray();

    Tokenize(TokenArrayID, Code);

    Print(StdOut, Str("Code = '"));
    Print(StdOut, Code);
    Print(StdOut, Str("'"));
    PrintNewLine(StdOut);

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

