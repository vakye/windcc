
// ==========================================================================================
// NOTE(vak): Printing functions for characters, strings, integers, ...
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
#include "platform.c"

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef usize print_write(void* Data, usize Size, void* UserData);

typedef struct
{
    print_write* Write;
    void*        UserData;
} print_out;

#define StdNil (print_out){&NilPrintWrite, 0}
#define StdOut (print_out){(print_write*)&WriteStdOut, 0}
#define StdErr (print_out){(print_write*)&WriteStdErr, 0}

local usize PrintCharacter  (print_out Out, char Character);
local usize PrintNewLine    (print_out Out);
local usize Print           (print_out Out, string Message);
local usize Println         (print_out Out, string Message);
local usize PrintUSize      (print_out Out, usize Value);
local usize PrintSSize      (print_out Out, ssize Value);
local usize RightPadOutput  (print_out Out, usize Written, usize Padding);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local usize NilPrintWrite(void* Data, usize Size, void* UserData)
{
    usize Result = Size;
    return (Result);
}

local usize PrintWrite(print_out Out, void* Data, usize Size)
{
    usize Result = Out.Write(Data, Size, Out.UserData);
    return (Result);
}

local usize PrintCharacter(print_out Out, char Character)
{
    usize Result = PrintWrite(Out, &Character, 1);
    return (Result);
}

local usize PrintNewLine(print_out Out)
{
    usize Result = PrintCharacter(Out, '\n');
    return (Result);
}

local usize Print(print_out Out, string Message)
{
    usize Result = PrintWrite(Out, Message.Data, Message.Size);
    return (Result);
}

local usize Println(print_out Out, string Message)
{
    usize Result = 0;

    Result += Print(Out, Message);
    Result += PrintNewLine(Out);

    return (Result);
}

local usize PrintUSize(print_out Out, usize Value)
{
    char Buffer[USizeBits] = {0};
    usize DigitIndex = ArrayCount(Buffer);
    usize DigitCount = 0;

    do
    {
        char Digit = '0' + (char)(Value % 10);

        Value /= 10;
        DigitIndex--;
        DigitCount++;

        Buffer[DigitIndex] = Digit;
    } while (Value);

    usize Result = Print(Out, StrData(Buffer + DigitIndex, DigitCount));

    return (Result);
}

local usize PrintSSize(print_out Out, ssize Value)
{
    usize Result = 0;

    if (Value < 0)
    {
        Result += PrintCharacter(Out, '-');
        Value = -Value;
    }

    Result += PrintUSize(Out, Value);

    return (Result);
}

local usize RightPadOutput(print_out Out, usize Written, usize Padding)
{
    while (Written < Padding)
    {
        Written += PrintCharacter(Out, ' ');
    }

    return (Written);
}

