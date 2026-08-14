
#pragma once

typedef u8 token_kind;
enum
{
    TokenKind_EOF,

    // NOTE(vak): Punctuation characters are mapped directly to their
    // ASCII codes. Alphabetical and digit ASCII codes are converted to
    // TokenKind_Identifier and TokenKind_Integer respectively, so they're
    // free for use.

    TokenKind_Integer       = '0',
    TokenKind_Identifier    = '1',
};

typedef struct token token;
struct token
{
    token_kind Kind;
    string String;
    token* Next;
};

local b32 IsPrintable(char Character)
{
    b32 Result = ((Character >= 32) && (Character <= 126));
    return (Result);
}

local b32 IsWhitespace(char Character)
{
    b32 Result =
        (Character == ' ') ||
        (Character == '\r') ||
        (Character == '\t') ||
        (Character == '\n');

    return (Result);
}

local b32 IsDigit(char Character)
{
    b32 Result = ((Character >= '0') && (Character <= '9'));
    return (Result);
}

local b32 IsAlphabet(char Character)
{
    b32 Result =
        ((Character >= 'a') && (Character <= 'z')) ||
        ((Character >= 'A') && (Character <= 'Z'));

    return (Result);
}

local b32 IsIdentifierStart(char Character)
{
    b32 Result = (Character == '_') || IsAlphabet(Character);
    return (Result);
}

local b32 IsIdentifier(char Character)
{
    b32 Result = IsIdentifierStart(Character) || IsDigit(Character);
    return (Result);
}

local token* Tokenize(string Code)
{
    token Sentinel = {0};
    token* Last = &Sentinel;

    usize Index = 0;
    while (Index < Code.Size)
    {
        while (Index < Code.Size)
        {
            if (!IsWhitespace(Code.Data[Index]))
                break;

            Index++;
        }

        if (Index == Code.Size)
            break;

        Last = Last->Next = Allocate(sizeof(token));

        usize From = Index;

        if (IsDigit(Code.Data[Index]))
        {
            Last->Kind = TokenKind_Integer;

            Index++;
            while (Index < Code.Size)
            {
                if (!IsDigit(Code.Data[Index]))
                    break;

                Index++;
            }
        }
        else if (IsIdentifierStart(Code.Data[Index]))
        {
            Last->Kind = TokenKind_Identifier;

            Index++;
            while (Index < Code.Size)
            {
                if (!IsIdentifier(Code.Data[Index]))
                    break;

                Index++;
            }
        }
        else if (IsPrintable(Code.Data[Index]))
        {
            Last->Kind = (token_kind)Code.Data[Index]; // NOTE(vak): Punctuation
            Index++;
        }
        else
        {
            Print(Str("ERROR: Unknown character '"));
            PrintCharacter(Code.Data[Index]);
            Print(Str("'"));
            PrintNewLine();
            Exit(1);
        }

        usize Size = Index - From;

        Last->String = StringView(Code, From, Size);
    }

    Last = Last->Next = Allocate(sizeof(token));
    Last->Kind = TokenKind_EOF;
    Last->String = Str("");

    token* First = Sentinel.Next;
    return (First);
}

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

