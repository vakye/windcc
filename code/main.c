
#pragma once

typedef ssize program_entry_point(void);

local ssize CompileAndRun(string Code)
{
    token* FirstToken = Tokenize(Code);
    node* RootNode = Parse(FirstToken);

    usize AssemblySize = Generate(0, 0, RootNode);
    void* Assembly = Allocate(AssemblySize);

    Generate(Assembly, AssemblySize, RootNode);

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
        {  1200,            StaticStr("  120/ 2*( 10+  10)  ") },
        {  610,             StaticStr("120 / 2*10 + 10") },
        {  603,             StaticStr("120 / 2*10 + 10 % 7") },
        {  3,               StaticStr("120 / 2*(10 + 10) % 7") },
        { -60,              StaticStr("(120 / 2) * (5 - 10 + 4)") },
        {  1,               StaticStr("120 / 2*(10 + 10) % 7 == 3") },
        {  1,               StaticStr("3 == 120 / 2*(10 + 10) % 7") },
        {  0,               StaticStr("3 != 120 / 2*(10 + 10) % 7") },
        {  1,               StaticStr("120 / 2*(10 + 10) % 7 != 6") },
        {  0,               StaticStr("12387 == 23781") },
        {  1,               StaticStr("12387 != 23781") },
        {  1,               StaticStr("1337 * 10 / 10 == 1337 + 10 - 10") },
        {  1,               StaticStr("1337 * 10 / 10 != 1337 + 10 - 5") },
        {  1,               StaticStr("10 + 1300 - 50 > 50 / 3") },
        {  0,               StaticStr("120 / (10 + 1300 - 50) > 50 / 300") },
        {  1,               StaticStr("120 / (10 + 1300 - 50) >= 50 / 300") },
        {  1,               StaticStr("120 / (10 + 1300 - 50) <= 50 / 300") },
        {  1,               StaticStr("120 / (10 + 1300 - 50) <= 50 / 3") },
        {  1,               StaticStr("120 / (10 + 1300 - 50) < 50 / 3") },
        {  1,               StaticStr("2 > 1") },
        {  0,               StaticStr("1 > 2") },
        {  1,               StaticStr("1 < 2") },
        {  0,               StaticStr("1 > 2") },
        {  0,               StaticStr("2 > 2") },
        {  0,               StaticStr("2 < 2") },
        {  1,               StaticStr("2 >= 2") },
        {  1,               StaticStr("2 <= 2") },
        {  1,               StaticStr("2 == 2") },
        {  0,               StaticStr("2 != 2") },
        {  0,               StaticStr("2 == 8") },
        {  1,               StaticStr("2 != 8") },
    };

    usize TestsPassed = 0;

    PrintNewLine();

    for (usize TestIndex = 0; TestIndex < ArrayCount(TestCases); TestIndex++)
    {
        test_case* TestCase = TestCases + TestIndex;

        Print(Str("TestCases["));
        RightPadOutput(PrintUSize(TestIndex), 2);
        Print(Str("]: "));

        ssize RunResult = CompileAndRun(TestCase->Code);
        b32 Passed = (RunResult == TestCase->Expected);

        TestsPassed += Passed;

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

    PrintNewLine();
    Print(Str("Tests passed: "));
    PrintUSize(TestsPassed);
    Print(Str("/"));
    PrintUSize(ArrayCount(TestCases));
    PrintNewLine();

    PrintNewLine();
}

