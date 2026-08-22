
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
#include "generator_x64.c"

// ==========================================================================================
// NOTE(vak): Main
// ==========================================================================================

typedef ssize program_entry(void);

local ssize CallMachineCode(void* Code, usize Size)
{
    memory_id ExecutableMemoryID = ReserveMemory(Size);

    memory_protection_flags ProtectionFlags =
        MemoryProtectionFlag_Readable   |
        MemoryProtectionFlag_Writeable  |
        MemoryProtectionFlag_Executable;

    CommitMemory(ExecutableMemoryID, 0, Size);
    ProtectMemory(ExecutableMemoryID, 0, Size, ProtectionFlags);

    CopyMemory(GetMemoryBase(ExecutableMemoryID), Code, Size);

    program_entry* ProgramEntry = (program_entry*)GetMemoryBase(ExecutableMemoryID);

    ssize Result = ProgramEntry();

    ReleaseMemory(ExecutableMemoryID);

    return (Result);
}

local void Main(void)
{
    string Code = Str("1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10");

    SetupLexer();
    SetupParser();
    x64_SetupGenerator();

    token_array Tokens          = Tokenize(Code);
    node_id     Node            = Parse(Tokens);
    x64_code    Generated       = x64_Generate(Node);
    usize       ExecutionResult = CallMachineCode(Generated.Base, Generated.Size);

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

    // NOTE(vak): Execution result
    {
        Print(StdOut, Str("Execution result: "));
        PrintSSize(StdOut, ExecutionResult);
        PrintNewLine(StdOut);
    }
}

