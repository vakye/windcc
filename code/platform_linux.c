
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

local memory_id ReserveMemory(usize Size)
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

    memory_id Result = {0};

    if (MapResult > 0)
    {
        Result.U64[0] = (usize)MapResult;
        Result.U64[1] = (usize)Size;
    }

    return (Result);
}

local b32 ReleaseMemory(memory_id MemoryID)
{
    if (IsNilMemoryID(MemoryID))
        return (false);

    usize Base = (usize)GetMemoryBase(MemoryID);
    usize Size = (usize)GetMemorySize(MemoryID);

    ssize ReleaseResult = LinuxSyscall(
        SyscallNumber_MUnMap,
        Base,
        Size,
        0, 0, 0, 0
    );

    b32 Result = (ReleaseResult >= 0);
    return (Result);
}

local void* GetMemoryBase(memory_id MemoryID)
{
    void* Result = (void*)(MemoryID.U64[0]);
    return (Result);
}

local usize GetMemorySize(memory_id MemoryID)
{
    usize Result = (MemoryID.U64[1]);
    return (Result);
}

local b32 CommitMemory(memory_id MemoryID, usize From, usize Size)
{
    if (IsNilMemoryID(MemoryID))
        return (false);

    if (From + Size > GetMemorySize(MemoryID))
        return (false);

    u8* CommitAt = (u8*)GetMemoryBase(MemoryID) + From;

    ssize CommitResult = (ssize)LinuxSyscall(
        SyscallNumber_MProtect,
        (usize)CommitAt,
        Size,
        PROT_READ|PROT_WRITE,
        0, 0, 0
    );

    b32 Result = (CommitResult >= 0);
    return (Result);
}

local b32 DecommitMemory(memory_id MemoryID, usize From, usize Size)
{
    if (IsNilMemoryID(MemoryID))
        return (false);

    if (From + Size > GetMemorySize(MemoryID))
        return (false);

    u8* DecommitAt = (u8*)GetMemoryBase(MemoryID) + From;

    ssize DecommitResult = (ssize)LinuxSyscall(
        SyscallNumber_MProtect,
        (usize)DecommitAt,
        Size,
        PROT_NONE,
        0, 0, 0
    );

    b32 Result = (DecommitResult >= 0);
    return (Result);
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

