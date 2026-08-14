
#pragma once

typedef ssize program_entry_point(void);

local void Main(void)
{
    string Code = Str("10237");

    usize ReturnNumber = 0;

    for (usize Index = 0; Index < Code.Size; Index++)
    {
        ReturnNumber *= 10;
        ReturnNumber += (Code.Data[Index] - '0');
    }

    u8 GeneratedAssembly[] =
    {
        // NOTE(vak):
        // 48 b8 (Imm64)    mov rax, Imm64
        // c3               ret

        0x48, 0xb8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0xc3,
    };

    CopyMemory(GeneratedAssembly + 2, &ReturnNumber, 8);

    usize ProgramSize = sizeof(GeneratedAssembly);

    program_entry_point* ProgramEntry = (program_entry_point*)
        MapExecutableMemory(GeneratedAssembly, ProgramSize);

    ssize ProgramResult = ProgramEntry();

    UnmapExecutableMemory((void*)ProgramEntry, ProgramSize);

    Print(Str("Execution result: "));
    PrintSSize(ProgramResult);
    PrintNewLine();
}

