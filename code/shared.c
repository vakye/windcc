
// ==========================================================================================
// NOTE(vak): A collection of definitions that are commonly used throughout the codebase.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Platform detection
// ==========================================================================================

#if defined(_WIN32) || defined(_WIN64)
#define PlatformWindows (1)
#elif defined(__linux__)
#define PlatformLinux (1)
#else
#error Unknown platform
#endif

#if !defined(PlatformWindows)
#define PlatformWindows (0)
#endif

#if !defined(PlatformLinux)
#define PlatformLinux (0)
#endif

// ==========================================================================================
// NOTE(vak): Architecture detection
// ==========================================================================================

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define ArchitectureX64 (1)
#else
#error Unknown architecture
#endif

#if !defined(ArchitectureX64)
#define ArchitectureX64 (0)
#endif

// ==========================================================================================
// NOTE(vak): Keywords
// ==========================================================================================

#define local static    // NOTE(vak): Marks a function or variable as being private to this program (not exported)
#define persist static  // NOTE(vak): Marks a variable as having persistent storage that retains its value even after the function returns.

#define CTAssert(Expression) _Static_assert(Expression, "Compile-time assertion failed")

#define AlwaysAssert(Expression) if (!(Expression)) { __builtin_trap(); }

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

#define AlignUp(Value, PowerOf2) (((Value) + (PowerOf2) - 1) & ~((PowerOf2) - 1))

// ==========================================================================================
// NOTE(vak): Integer types
// ==========================================================================================

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef s64 ssize;
typedef u64 usize;

// ==========================================================================================
// NOTE(vak): Floating point types
// ==========================================================================================

typedef float f32;
typedef double f64;

// ==========================================================================================
// NOTE(vak): Boolean types
// ==========================================================================================

typedef u8 b8;
typedef u32 b32;

// ==========================================================================================
// NOTE(vak): Constants
// ==========================================================================================

#define true  (1)
#define false (0)

#define S8Min ((s8)(0x80))
#define S8Max ((s8)(0x7F))

#define S16Min ((s16)(0x8000))
#define S16Max ((s16)(0x7FFF))

#define S32Min ((s32)(0x80000000))
#define S32Max ((s32)(0x7FFFFFFF))

#define S64Min ((s64)(0x8000000000000000))
#define S64Max ((s64)(0x7FFFFFFFFFFFFFFF))

#define U8Max ((u8)(0xFF))
#define U16Max ((u16)(0xFFFF))
#define U32Max ((u32)(0xFFFFFFFF))
#define U64Max ((u64)(0xFFFFFFFFFFFFFFFF))

#define USizeBits (sizeof(usize) * 8)
#define USizeMax  (~((usize)0))

// ==========================================================================================
// NOTE(vak): Type size checks
// ==========================================================================================

CTAssert(sizeof(s8)  == 1);
CTAssert(sizeof(s16) == 2);
CTAssert(sizeof(s32) == 4);
CTAssert(sizeof(s64) == 8);

CTAssert(sizeof(u8)  == 1);
CTAssert(sizeof(u16) == 2);
CTAssert(sizeof(u32) == 4);
CTAssert(sizeof(u64) == 8);

CTAssert(sizeof(ssize) == sizeof(void*));
CTAssert(sizeof(usize) == sizeof(void*));

CTAssert(sizeof(f32) == 4);
CTAssert(sizeof(f64) == 8);

// ==========================================================================================
// NOTE(vak): Memory
// ==========================================================================================

// NOTE(vak): We specified that we don't want to use the CRT, so the compiler expects
// us to provide memset and memcpy. That is why we have to implement it ourselves here.

void* memset(void* DestInit, s32 Byte, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = (u8)Byte;

    return (DestInit);
}

void* memcpy(void* DestInit, void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;

    return (DestInit);
}

#define ZeroType(Pointer)           ZeroMemory(Pointer, sizeof(*(Pointer)))
#define ZeroArray(Pointer, Count)   ZeroMemory(Pointer, sizeof(*(Pointer)) * (Count))

local void ZeroMemory(void* DestInit, usize Size)                   { memset(DestInit, 0, Size); }
local void FillMemory(void* DestInit, u8 Byte, usize Size)          { memset(DestInit, Byte, Size); }
local void CopyMemory(void* DestInit, void* SourceInit, usize Size) { memcpy(DestInit, SourceInit, Size); }

// ==========================================================================================
// NOTE(vak): String
// ==========================================================================================

typedef struct
{
    char* Data;
    usize Size;
} string;

#define StaticStr(Literal) {Literal, sizeof(Literal) - 1}
#define StaticStrData(Data, Size) {Data, Size}

#define Str(Literal) (string){Literal, sizeof(Literal) - 1}
#define StrData(Data, Size) (string){Data, Size}

#define NilString (string){0}

local string StringView(string String, usize From, usize Size)
{
    AlwaysAssert(From + Size <= String.Size);

    string Result = StrData(String.Data + From, Size);
    return (Result);
}

local b32 StringEqual(string A, string B)
{
    b32 Result = (A.Size == B.Size);

    if (Result)
    {
        for (usize Index = 0; Index < A.Size; Index++)
        {
            if (A.Data[Index] != B.Data[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return (Result);
}

