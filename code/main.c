
// ==========================================================================================
// NOTE(vak): Main program logic
// ==========================================================================================

#pragma once

#include "memory.c"
#include "print.c"
#include "character.c"
#include "lexer.c"

local void Main(void)
{
    string Code = Str("int main () { __hello + world_23_() * (10 + 10) / 120; }");

    token_array_id TokenArrayID = CreateTokenArray();

    Tokenize(TokenArrayID, Code);

    Print(Str("Code = '"));
    Println(Code);

    Println(Str("Tokenizer output:"));

    u32 TokenCount = GetTokenCount(TokenArrayID);
    for (token_id TokenID = 0; TokenID < TokenCount; TokenID++)
    {
        Print(Str("    "));
        Print(Str("TokenID = "));
        RightPadOutput(PrintUSize(TokenID), 16);
        Print(Str("String = '"));
        Print(GetTokenString(TokenArrayID, TokenID));
        Print(Str("'"));
        PrintNewLine();
    }
}

