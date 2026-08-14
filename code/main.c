
#pragma once

local usize TokenToInteger(token* Token)
{
    // NOTE(vak): Assuming Token->Kind == TokenKind_Integer

    usize Result = 0;

    for (usize Index = 0; Index < Token->String.Size; Index++)
    {
        Result *= 10;
        Result += (Token->String.Data[Index] - '0');
    }

    return (Result);
}

typedef ssize program_entry_point(void);

local ssize CompileAndRun(string Code)
{
    token* Token = Tokenize(Code);

    usize NumberA = 0;
    char Operator = 0;
    usize NumberB = 0;

    if (Token->Kind != TokenKind_Integer)
    {
        Println(Str("ERROR: Expected an integer"));
        Exit(1);
    }

    NumberA = TokenToInteger(Token);
    Token = Token->Next;

    switch (Token->Kind)
    {
        default:
        {
            Println(Str("ERROR: Syntax error"));
            Exit(1);
        } break;

        case TokenKind_EOF:
        {
        } break;

        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        {
            Operator = Token->Kind;
            Token = Token->Next;

            if (Token->Kind != TokenKind_Integer)
            {
                Println(Str("ERROR: Expected an integer"));
                Exit(1);
            }

            NumberB = TokenToInteger(Token);
            Token = Token->Next;
        } break;
    }

    if (Token->Kind != TokenKind_EOF)
    {
        Println(Str("ERROR: Syntax error"));
        Exit(1);
    }

    u8 Assembly[256] = {0};
    usize AssemblySize = 0;

    // NOTE(vak):
    // 48 b8 (Imm64) mov rax, NumberA
    {
        Assembly[AssemblySize++] = 0x48;
        Assembly[AssemblySize++] = 0xb8;
        CopyMemory(Assembly + AssemblySize, &NumberA, 8);
        AssemblySize += 8;
    }

    if (Operator)
    {
        // NOTE(vak):
        // 48 b9 (Imm64) mov rcx, NumberB
        {
            Assembly[AssemblySize++] = 0x48;
            Assembly[AssemblySize++] = 0xb9;
            CopyMemory(Assembly + AssemblySize, &NumberB, 8);
            AssemblySize += 8;
        }

        u64 Instructions = 0;
        u64 InstructionsSize = 0;

        switch (Operator)
        {
            default:
            {
                Print(Str("ERROR: '"));
                PrintCharacter(Operator);
                Print(Str("' is not a valid operator."));
                PrintNewLine();
                Exit(1);
            } break;

            case '\0':
            {
                // NOTE(vak): No operator, return a single number
            } break;

            // NOTE(vak):
            // 48 03 c1         add rax, rcx
            case '+': Instructions = 0xc10348; InstructionsSize = 3; break;

            // NOTE(vak):
            // 48 2b c1         sub rax, rcx
            case '-': Instructions = 0xc12b48; InstructionsSize = 3; break;

            // NOTE(vak):
            // 48 0f af c1     imul rax, rcx
            case '*': Instructions = 0xc1af0f48; InstructionsSize = 4; break;

            // NOTE(vak):
            // 48 99            cqo
            // 48 f7 f9         idiv rcx
            case '/': Instructions = 0xf9f7489948; InstructionsSize = 5; break;

            // NOTE(vak):
            // 48 99            cqo
            // 48 f7 f9         idiv rcx
            // 48 8b c2         mov rax, rdx
            case '%': Instructions = 0xc28b48f9f7489948; InstructionsSize = 8; break;
        }

        CopyMemory(Assembly + AssemblySize, &Instructions, InstructionsSize);
        AssemblySize += InstructionsSize;
    }

    // NOTE(vak):
    // c3 ret
    {
        Assembly[AssemblySize++] = 0xc3;
    }

    program_entry_point* ProgramEntry = (program_entry_point*)
        MapExecutableMemory(Assembly, AssemblySize);

    ssize ProgramResult = ProgramEntry();

    UnmapExecutableMemory((void*)ProgramEntry, AssemblySize);

    return (ProgramResult);
}

typedef struct
{
    ssize Expected;
    string Code;
} test_case;

local usize RightPadOutput(usize Written, usize Padding)
{
    while (Written < Padding)
        Written += PrintCharacter(' ');

    return (Written);
}

local void Main(void)
{
    test_case TestCases[] =
    {
        {  1337,            StaticStr("1337") },
        {  10,              StaticStr("5 +5") },
        {  50,              StaticStr("5* 10") },
        {  35,              StaticStr("50 -   15") },
        { -1247,            StaticStr("  753 - 2000") },
        {  47,              StaticStr(" 10+37   ") },
        {  20,              StaticStr("  100 /5 ") },
        {  1,               StaticStr("5 / 3") },
        {  4,               StaticStr("28471824 % 13") },
        {  2,               StaticStr("284 % 3") },
    };

    for (usize TestIndex = 0; TestIndex < ArrayCount(TestCases); TestIndex++)
    {
        test_case* TestCase = TestCases + TestIndex;

        Print(Str("TestCases["));
        PrintUSize(TestIndex);
        Print(Str("]: "));

        ssize RunResult = CompileAndRun(TestCase->Code);
        b32 Passed = (RunResult == TestCase->Expected);

        usize Padding = 16;

        Print(Passed ? Str("\033[32m") : Str("\033[31m"));
        RightPadOutput(Print(Passed ? Str("PASSED") : Str("FAILED")), Padding);
        Print(Str("\033[0m"));

        Print(Str("RunResult = "));
        RightPadOutput(PrintSSize(RunResult), Padding);

        Print(Str("Expected = "));
        RightPadOutput(PrintSSize(TestCase->Expected), Padding);

        Print(Str("Code = '"));
        Print(TestCase->Code);
        Print(Str("'"));

        PrintNewLine();
    }
}

