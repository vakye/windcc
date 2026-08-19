
#pragma once

local usize PrintCharacter(char Character)
{
    usize Result = WriteStdOut(&Character, 1);
    return (Result);
}

local usize PrintNewLine(void)
{
    usize Result = PrintCharacter('\n');
    return (Result);
}

local usize Print(string Message)
{
    usize Result = WriteStdOut(Message.Data, Message.Size);
    return (Result);
}

local usize Println(string Message)
{
    usize Result = 0;

    Result += Print(Message);
    Result += PrintNewLine();

    return (Result);
}

local usize PrintUSize(usize Value)
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

    usize Result = Print(StrData(Buffer + DigitIndex, DigitCount));

    return (Result);
}

local usize PrintSSize(ssize Value)
{
    usize Result = 0;

    if (Value < 0)
    {
        Result += PrintCharacter('-');
        Value = -Value;
    }

    Result += PrintUSize(Value);

    return (Result);
}

local usize RightPadOutput(usize Written, usize Padding)
{
    while (Written < Padding)
        Written += PrintCharacter(' ');

    return (Written);
}

