
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
        {  1337,                    StaticStr("1337;") },
        {  10,                      StaticStr("5 +5;") },
        {  50,                      StaticStr("5* 10;") },
        {  35,                      StaticStr("50 -   15 ;") },
        { -1247,                    StaticStr("  753 - 2000;") },
        {  47,                      StaticStr(" 10+37   ;") },
        {  20,                      StaticStr("  100 /5 ;") },
        {  1,                       StaticStr("5 / 3;") },
        {  4,                       StaticStr("28471824 % 13;") },
        {  2,                       StaticStr("284 % 3;") },
        {  1200,                    StaticStr("  120/ 2*( 10+  10)  ;") },
        {  610,                     StaticStr("120 / 2*10 + 10;") },
        {  603,                     StaticStr("120 / 2*10 + 10 % 7;") },
        {  3,                       StaticStr("120 / 2*(10 + 10) % 7;") },
        { -60,                      StaticStr("(120 / 2) * (5 - 10 + 4);") },
        {  1,                       StaticStr("120 / 2*(10 + 10) % 7 == 3;") },
        {  1,                       StaticStr("3 == 120 / 2*(10 + 10) % 7;") },
        {  0,                       StaticStr("3 != 120 / 2*(10 + 10) % 7;") },
        {  1,                       StaticStr("120 / 2*(10 + 10) % 7 != 6;") },
        {  0,                       StaticStr("12387 == 23781;") },
        {  1,                       StaticStr("12387 != 23781;") },
        {  1,                       StaticStr("1337 * 10 / 10 == 1337 + 10 - 10;") },
        {  1,                       StaticStr("1337 * 10 / 10 != 1337 + 10 - 5;") },
        {  1,                       StaticStr("10 + 1300 - 50 > 50 / 3;") },
        {  0,                       StaticStr("120 / (10 + 1300 - 50) > 50 / 300;") },
        {  1,                       StaticStr("120 / (10 + 1300 - 50) >= 50 / 300;") },
        {  1,                       StaticStr("120 / (10 + 1300 - 50) <= 50 / 300;") },
        {  1,                       StaticStr("120 / (10 + 1300 - 50) <= 50 / 3;") },
        {  1,                       StaticStr("120 / (10 + 1300 - 50) < 50 / 3;") },
        {  1,                       StaticStr("2 > 1;") },
        {  0,                       StaticStr("1 > 2;") },
        {  1,                       StaticStr("1 < 2;") },
        {  0,                       StaticStr("1 > 2;") },
        {  0,                       StaticStr("2 > 2;") },
        {  0,                       StaticStr("2 < 2;") },
        {  1,                       StaticStr("2 >= 2;") },
        {  1,                       StaticStr("2 <= 2;") },
        {  1,                       StaticStr("2 == 2;") },
        {  0,                       StaticStr("2 != 2;") },
        {  0,                       StaticStr("2 == 8;") },
        {  1,                       StaticStr("2 != 8;") },
        {  1024,                    StaticStr("1 << 10;") },
        {  512,                     StaticStr("1 << 10 >> 1; ") },
        {  1,                       StaticStr("(1 << 10) / 1024; ") },
        {  MB(20),                  StaticStr("(10 + 10) << 20; ") },
        { -156,                     StaticStr("100 - (1 << 10 >> 2);") },
        {  8192,                    StaticStr("1 + 1 << 4 + 8;") },
        {  8192,                    StaticStr("1 + (10 == 10) << (4 < 8)*4 + 8;") },
        {  2,                       StaticStr("1 << 1;") },
        {  16,                      StaticStr("64 >> 2;") },
        { -1,                       StaticStr("-1;") },
        { -398,                     StaticStr(" - +-  +- 398;") },
        { ~1,                       StaticStr("~1;") },
        {  832,                     StaticStr("~2237 & 1000;") },
        {  833,                     StaticStr("~2237 & 1000 | 1;") },
        {  833,                     StaticStr("1 ^ 1 | ~2237 & 1000 ^ 1 + ~-1;") },
        {  1,                       StaticStr("5 + -10 == -5;") },
        {  1,                       StaticStr("-2398 < 100;") },
        {  -1,                      StaticStr("~1 + 1;") },
        {  -1397,                   StaticStr("~1397 + 1;") },
        {  1,                       StaticStr("!0;") },
        {  0,                       StaticStr("!1;") },
        {  0,                       StaticStr("!12874;") },
        {  0,                       StaticStr("!-~~~0;") },
        {  0,                       StaticStr("10 ^ 10;") },
        {  1022,                    StaticStr("1023 ^ 1;") },
        {  1,                       StaticStr("1023 ^ 1022;") },
        {  10,                      StaticStr("2 | 8;") },
        {  13,                      StaticStr("1 ^ 1 ^ 1 | 4 | 8;") },
        {  1,                       StaticStr("12481248 && 23273;") },
        {  0,                       StaticStr("0 && 23273;") },
        {  0,                       StaticStr("123872 && 0;") },
        {  1,                       StaticStr("248274 && 2487 || 0;") },
        {  1,                       StaticStr("0 || 1238387 || 10;") },
        {  1,                       StaticStr("23487 || 0 && 2387;") },
        {  0,                       StaticStr("0 || 1 ^ 1;") },
        {  0,                       StaticStr("1 ^ 1 || 0 && 0;") },
        {  10,                      StaticStr("1 ? 10 : 20;") },
        {  20,                      StaticStr("0 ? 10 : 20;") },
        {  30,                      StaticStr("1 ? 10 ? 30 : 10 : 20;") },
        {  40,                      StaticStr("0 ? 10 ? 30 : 10 : 0 ? 30 : 40;") },
        {  1,                       StaticStr("10 >= 10 ? 1000 - 999 : 20;") },
        {  100,                     StaticStr("; 10; ; 20;;; 100; ;;") },
        {  100,                     StaticStr("int Hello = 50; int World = 30; Hello + World*2 - 10;") },
        {  25,                      StaticStr("int A; int B; int C; int D; int E; A = B = C = D = E = 5; A = 1; B = 2; A*5 + B*B + 1 + C + D + E;") },
        {  10,                      StaticStr("int A; int B; A = B = 3; A += 4; A + B;") },
        {  7,                       StaticStr("int A; int B; A = B = 3; A += 4; B -= 3; A + B;") },
        {  1,                       StaticStr("int A; int B; A = B = 3; A += 4; B -= 3; A %= 6; A + B;") },
        {  321,                     StaticStr("int A = 20; A <<= 10; A >>= 5; A *= 8; A += 10; A -= 5; A %= 273; A ^= 2481; A |= 1; A &= 341;") },
        {  10,                      StaticStr("int A = 0; A++; A++; A--; ++A; ++A; --A; A*5;") },
        {  1,                       StaticStr("int A = 0; ++A;") },
        {  10,                      StaticStr("int A = 10; A++;") },
        {  9,                       StaticStr("int A = 10; --A;") },
        {  50,                      StaticStr("int A = 50; A--;") },
        {  0,                       StaticStr("int A = 4294967296; A;") },
        {  0,                       StaticStr("char A = 256; A;") },
        {  117,                     StaticStr("char A = -128; A -= 11; A;") },
        {  -32768,                  StaticStr("short A = 32767; A += 1; A;") },
        { -1,                       StaticStr("char A = 255; A;") },
        { -1,                       StaticStr("signed char int A = 255; A;") },
        {  255,                     StaticStr("unsigned char A = 255; A;") },
        {  65535,                   StaticStr("unsigned short int A = 65535; A;") },
        { -1,                       StaticStr("signed short A = 65535; A;") },
        { -1,                       StaticStr("short int A = 65535; A;") },
        { 9223372036854775807,      StaticStr("long long int A = 9223372036854775807; A;") },
        { 4294967296,               StaticStr("unsigned int B = -1; unsigned long long A = B; A += 1; A;") },
        { 10,                       StaticStr("int A = 5; if (A == 5) 10; else 2;") },
        { 10,                       StaticStr("int A = 5; if (A == 5) { 10; } else { 2; }") },
        { 2,                        StaticStr("int A = 5; if (A != 5) { 10; } else { 2; }") },
        { 7,                        StaticStr("int A = 4; if (A == 5) { 10; } else if (A == 4) { 7; } else 2;") },
        { 2,                        StaticStr("int A = 3; if (A == 5) { 10; } else if (A == 4) { 7; } else 2;") },
        { 10,                       StaticStr("int A = 3; { int B = 5; } int B = 7; A + B;") },
        { 10,                       StaticStr("int A = 0; for (int I = 0; I < 10; I++) { A++; } A;") },
        { 6765,                     StaticStr("int A = 0; int B = 1; for (int I = 0; I < 10; I++) { int C = A + B; B = A; A = C; } for (int I = 0; I < 10; I++) { int C = A + B; B = A; A = C; } A;") },
        { 10,                       StaticStr("int A = 0; while (A < 10) { A++; } A;") },
        { 30,                       StaticStr("int A = 0; int B = 0; while (B < 10) { A += 2; ++B; } A + B;") },
        { 2,                        StaticStr("int A = 10; int B = 0; do { A /= 10; B++; } while (A); B;") },
        { 1,                        StaticStr("int A = 0; int B = 0; do { A /= 10; B++; } while (A); B;") },
        { 2,                        StaticStr("int A = 10; int B = 0; while(A) { A /= 10; B++; } B;") },
        { 0,                        StaticStr("int A = 0; int B = 0; while(A) { A /= 10; B++; } B;") },
        { 10,                       StaticStr("int A = 0; while(A < 100) { if (A < 10) A++; else break; } A;") },
        { 30,                       StaticStr("int A = 0; for (int I = 0; I < 100; I++) { if (I < 20) { continue; } else if (I < 50) { A++; continue; } else break; A += 10; } A;") },
        { 10,                       StaticStr("int A = 0; *(&A) = 10; *(&A);") },
        { 40,                       StaticStr("int A = 0; int* B = &A; *B = 40; *B;") },
        { 50,                       StaticStr("int A = 0; int* B = &A; int** C = &B; **C = 50; **C;") },
    };

    usize TestsPassed = 0;

    PrintNewLine();

    for (usize TestIndex = 0; TestIndex < ArrayCount(TestCases); TestIndex++)
    {
        test_case* TestCase = TestCases + TestIndex;

        Print(Str("TestCases["));
        RightPadOutput(PrintUSize(TestIndex), 3);
        Print(Str("]: "));

        ssize RunResult = CompileAndRun(TestCase->Code);
        b32 Passed = (RunResult == TestCase->Expected);

        TestsPassed += Passed;

        usize Padding = 26;

        Print(Passed ? Str("\033[32m") : Str("\033[31m"));
        RightPadOutput(Print(Passed ? Str("PASSED") : Str("FAILED")), 12);
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

