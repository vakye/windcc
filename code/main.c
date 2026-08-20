
// ==========================================================================================
// NOTE(vak): Main program logic
// ==========================================================================================

#pragma once

local void Main(void)
{
    string Code = Str("1 + 2 + 3 + 4 + 5");

    EquipLexerCode(Code);

    node* Node = Parse();
}

