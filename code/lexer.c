
#pragma once

typedef u8 token_kind;
enum
{
    TokenKind_EOF,

    // NOTE(vak): Punctuation characters are mapped directly to their
    // ASCII codes. Alphabetical and digit ASCII codes are converted to
    // TokenKind_Identifier and TokenKind_Integer respectively, so they're
    // free for use.

    TokenKind_Integer               = '0',
    TokenKind_Identifier            = '1',
    TokenKind_DoubleEqual           = '2', // NOTE(vak): "=="
    TokenKind_BangEqual             = '3', // NOTE(vak): "!="
    TokenKind_LessEqual             = '4', // NOTE(vak): "<="
    TokenKind_GreaterEqual          = '5', // NOTE(vak): ">="
    TokenKind_DoubleLess            = '6', // NOTE(vak): "<<"
    TokenKind_DoubleGreater         = '7', // NOTE(vak): ">>"
    TokenKind_DoubleAmpersand       = '8', // NOTE(vak): "&&"
    TokenKind_DoubleBar             = '9', // NOTE(vak): "||"

    TokenKind_PlusEqual             = 'a', // NOTE(vak): "+="
    TokenKind_MinusEqual            = 'b', // NOTE(vak): "-="
    TokenKind_StarEqual             = 'c', // NOTE(vak): "*="
    TokenKind_SlashEqual            = 'd', // NOTE(vak): "/="
    TokenKind_PercentEqual          = 'e', // NOTE(vak): "%="
    TokenKind_DoubleLessEqual       = 'f', // NOTE(vak): "<<="
    TokenKind_DoubleGreaterEqual    = 'g', // NOTE(vak): ">>="
    TokenKind_AmpersandEqual        = 'h', // NOTE(vak): "&="
    TokenKind_HatEqual              = 'i', // NOTE(vak): "^="
    TokenKind_BarEqual              = 'j', // NOTE(vak): "|="
    TokenKind_DoublePlus            = 'k', // NOTE(vak): "++"
    TokenKind_DoubleMinus           = 'l', // NOTE(vak): "--"

    TokenKind_Int                   = 'A', // NOTE(vak): "int"
    TokenKind_Char                  = 'B', // NOTE(vak): "char"
    TokenKind_Short                 = 'C', // NOTE(vak): "short"
    TokenKind_Signed                = 'D', // NOTE(vak): "signed"
    TokenKind_Unsigned              = 'E', // NOTE(vak): "unsigned"
    TokenKind_Long                  = 'F', // NOTE(vak): "long"
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
            // NOTE(vak): Integer

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
            // NOTE(vak): Identifier

            Last->Kind = TokenKind_Identifier;

            Index++;
            while (Index < Code.Size)
            {
                if (!IsIdentifier(Code.Data[Index]))
                    break;

                Index++;
            }

            usize Size = Index - From;
            string Slice = StringView(Code, From, Size);

            if (StringEqual(Slice, Str("int")))
                Last->Kind = TokenKind_Int;
            else if (StringEqual(Slice, Str("char")))
                Last->Kind = TokenKind_Char;
            else if (StringEqual(Slice, Str("short")))
                Last->Kind = TokenKind_Short;
            else if (StringEqual(Slice, Str("unsigned")))
                Last->Kind = TokenKind_Unsigned;
            else if (StringEqual(Slice, Str("signed")))
                Last->Kind = TokenKind_Signed;
            else if (StringEqual(Slice, Str("long")))
                Last->Kind = TokenKind_Long;
        }
        else if (IsPrintable(Code.Data[Index]))
        {
            // NOTE(vak): Punctuation

            char Character = Code.Data[Index];
            Index++;

            Last->Kind = (token_kind)Character;

            u32 MatchValue = Character;

            if (Index + 1 <= Code.Size)
            {
                MatchValue |= (u32)Code.Data[Index] << 8;

                if (Index + 2 <= Code.Size)
                    MatchValue |= (u32)Code.Data[Index + 1] << 16;
            }

            b32 AlreadyMatched = true;

            // NOTE(vak): 3 character operators

            switch (MatchValue)
            {
                default: AlreadyMatched = false; break;

                #define MatchCase(C0, C1, C2, MatchToKind) \
                    case (C0) | (C1 << 8) | (C2 << 16): Last->Kind = MatchToKind; Index += 2; break

                MatchCase('<', '<', '=', TokenKind_DoubleLessEqual);
                MatchCase('>', '>', '=', TokenKind_DoubleGreaterEqual);

                #undef MatchCase
            }

            if (!AlreadyMatched)
            {
                // NOTE(vak): 2 character operators

                switch (MatchValue & 0xFFFF)
                {
                    #define MatchCase(C0, C1, MatchToKind) \
                        case (C0) | (C1 << 8): Last->Kind = MatchToKind; Index++; break

                    MatchCase('=', '=', TokenKind_DoubleEqual);
                    MatchCase('!', '=', TokenKind_BangEqual);
                    MatchCase('<', '=', TokenKind_LessEqual);
                    MatchCase('>', '=', TokenKind_GreaterEqual);

                    MatchCase('<', '<', TokenKind_DoubleLess);
                    MatchCase('>', '>', TokenKind_DoubleGreater);

                    MatchCase('&', '&', TokenKind_DoubleAmpersand);
                    MatchCase('|', '|', TokenKind_DoubleBar);

                    MatchCase('+', '=', TokenKind_PlusEqual);
                    MatchCase('-', '=', TokenKind_MinusEqual);
                    MatchCase('*', '=', TokenKind_StarEqual);
                    MatchCase('/', '=', TokenKind_SlashEqual);
                    MatchCase('%', '=', TokenKind_PercentEqual);
                    MatchCase('&', '=', TokenKind_AmpersandEqual);
                    MatchCase('^', '=', TokenKind_HatEqual);
                    MatchCase('|', '=', TokenKind_BarEqual);

                    MatchCase('+', '+', TokenKind_DoublePlus);
                    MatchCase('-', '-', TokenKind_DoubleMinus);

                    #undef MatchCase
                }
            }
        }
        else
        {
            u8 Character = (u8)Code.Data[Index];

            Print(Str("ERROR: Unknown character \\"));
            PrintUSize(Character);
            Print(Str(" in input."));
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

