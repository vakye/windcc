
// ==========================================================================================
// NOTE(vak): Linux implementation of platform.c
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Standard file numbers
// ==========================================================================================

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

// ==========================================================================================
// NOTE(vak): Memory map constants
// ==========================================================================================

#define PROT_NONE   (0x00)
#define PROT_READ   (0x01)
#define PROT_WRITE  (0x02)
#define PROT_EXEC   (0x04)

#define MAP_PRIVATE     (0x02)
#define MAP_ANONYMOUS   (0x20)

// ==========================================================================================
// NOTE(vak): System call
// ==========================================================================================

typedef enum
{
#if ArchitectureX64
    SyscallNumber_Write     = 1,
    SyscallNumber_MMap      = 9,
    SyscallNumber_MProtect  = 10,
    SyscallNumber_MUnMap    = 11,
    SyscallNumber_Exit      = 60,
#else
#error Linux syscall numbers are not defined for this architecture
#endif
} syscall_number;

local usize LinuxSyscall(
    syscall_number SyscallNumber,
    usize Arg0, usize Arg1, usize Arg2,
    usize Arg3, usize Arg4, usize Arg5
)
{
    usize Result = 0;

#if ArchitectureX64
    register usize R10 __asm__("r10") = Arg3;
    register usize R8  __asm__("r8")  = Arg4;
    register usize R9  __asm__("r9")  = Arg5;

    __asm__ volatile (
        "syscall" :
        "=a"(Result) :
        "a"(SyscallNumber),
        "D"(Arg0),
        "S"(Arg1),
        "d"(Arg2),
        "r"(R10),
        "r"(R8),
        "r"(R9) :
        "memory", "rcx", "r11"
    );
#else
#error LinuxSyscall is not implemented for this architecture
#endif

    return (Result);
}

// ==========================================================================================
// NOTE(vak): Memory
// ==========================================================================================

local void* ReserveMemory(usize Size)
{
    ssize MapResult = (ssize)LinuxSyscall(
        SyscallNumber_MMap,
        0,
        Size,
        PROT_NONE,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1,
        0
    );

    if (MapResult < 0)
        Exit(-MapResult);

    void* Result = (void*)MapResult;
    return (Result);
}

local void CommitMemory(void* Memory, usize Size)
{
    ssize CommitResult = (ssize)LinuxSyscall(
        SyscallNumber_MProtect,
        (usize)Memory,
        Size,
        PROT_READ|PROT_WRITE,
        0, 0, 0
    );

    if (CommitResult < 0)
        Exit(-CommitResult);
}

local void* MapExecutableMemory(void* Data, usize Size)
{
    ssize MapResult = (ssize)LinuxSyscall(
        SyscallNumber_MMap,
        0,
        Size,
        PROT_READ|PROT_WRITE|PROT_EXEC,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1,
        0
    );

    if (MapResult < 0)
        Exit(-MapResult);

    void* Result = (void*)MapResult;

    CopyMemory(Result, Data, Size);

    return (Result);
}

local void UnmapExecutableMemory(void* Memory, usize Size)
{
    ssize UnmapResult = (ssize)LinuxSyscall(
        SyscallNumber_MUnMap,
        (usize)Memory,
        Size,
        0, 0, 0, 0
    );

    if (UnmapResult < 0)
        Exit(-UnmapResult);
}

// ==========================================================================================
// NOTE(vak): Console
// ==========================================================================================

local usize WriteStdOut(void* Data, usize Size, ...)
{
    ssize BytesWritten = (ssize)LinuxSyscall(
        SyscallNumber_Write,
        STDOUT_FILENO,
        (usize)Data,
        Size,
        0, 0, 0
    );

    usize Result = Maximum(0, BytesWritten);
    return (Result);
}

local usize WriteStdErr(void* Data, usize Size, ...)
{
    ssize BytesWritten = (ssize)LinuxSyscall(
        SyscallNumber_Write,
        STDERR_FILENO,
        (usize)Data,
        Size,
        0, 0, 0
    );

    usize Result = Maximum(0, BytesWritten);
    return (Result);
}

// ==========================================================================================
// NOTE(vak): Process control
// ==========================================================================================

local void Exit(u8 ExitCode)
{
    LinuxSyscall(SyscallNumber_Exit, ExitCode, 0, 0, 0, 0, 0);
}

