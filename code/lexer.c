
// ==========================================================================================
// NOTE(vak): Compiler lexer: Converts a string into a sequence of tokens for the parser.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

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
    TokenKind_If                    = 'G', // NOTE(vak): "if"
    TokenKind_Else                  = 'H', // NOTE(vak): "else"
    TokenKind_For                   = 'I', // NOTE(vak): "for"
    TokenKind_While                 = 'J', // NOTE(vak): "while"
    TokenKind_Do                    = 'K', // NOTE(vak): "do"
    TokenKind_Break                 = 'L', // NOTE(vak): "break"
    TokenKind_Continue              = 'M', // NOTE(vak): "continue"
};

typedef struct token token;
struct token
{
    token_kind Kind;
    string String;
    token* Next;
};

local void EquipLexerCode(string Code);
local token GetCurrentToken(void);
local b32 MatchToken(token_kind Kind);
local b32 NextIfMatchToken(token_kind Kind);
local void NextToken(void);
local usize TokenToInteger(token Token);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

typedef struct
{
    string Code;
    usize Index;
    token CurrentToken;
} lexer;

local lexer Lexer = {0};

local void EquipLexerCode(string Code)
{
    Lexer.Code = Code;
    Lexer.Index = 0;

    NextToken(); // NOTE(vak): Initializes Lexer.CurrentToken
}

local token GetCurrentToken(void)
{
    return (Lexer.CurrentToken);
}

local b32 MatchToken(token_kind Kind)
{
    b32 Result = (Lexer.CurrentToken.Kind == Kind);
    return (Result);
}

local b32 NextIfMatchToken(token_kind Kind)
{
    b32 Result = MatchToken(Kind);
    if (Result)
        NextToken();

    return (Result);
}

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

local char PeekCharacter(void)
{
    char Character = 0;

    if (Lexer.Index < Lexer.Code.Size)
    {
        Character = Lexer.Code.Data[Lexer.Index];
    }

    return (Character);
}

local char PeekAheadCharacter(usize Offset)
{
    char Character = 0;

    if (Lexer.Index + Offset < Lexer.Code.Size)
    {
        Character = Lexer.Code.Data[Lexer.Index + Offset];
    }

    return (Character);
}

local void ConsumeCharacter(void)
{
    if (Lexer.Index < Lexer.Code.Size)
    {
        Lexer.Index++;
    }
}

local void SkipWhitespace(void)
{
    while (Lexer.Index < Lexer.Code.Size)
    {
        char Character = PeekCharacter();

        if (!IsWhitespace(Character))
            break;

        ConsumeCharacter();
    }
}

local token_kind TokenizeDigit(void)
{
    token_kind Result = TokenKind_Integer;

    ConsumeCharacter();

    while (Lexer.Index < Lexer.Code.Size)
    {
        char Character = PeekCharacter();

        if (!IsDigit(Character))
            break;

        ConsumeCharacter();
    }

    return (Result);
}

typedef struct
{
    token_kind Kind;
    string String;
} keyword_mapping;

local token_kind TokenizeIdentifier(void)
{
    token_kind Result = TokenKind_Identifier;

    usize From = Lexer.Index;

    ConsumeCharacter();

    while (Lexer.Index < Lexer.Code.Size)
    {
        char Character = PeekCharacter();

        if (!IsIdentifier(Character))
            break;

        ConsumeCharacter();
    }

    persist keyword_mapping KeywordMappings[] =
    {
        { TokenKind_Int,            StaticStr("int") },
        { TokenKind_Char,           StaticStr("char") },
        { TokenKind_Short,          StaticStr("short") },
        { TokenKind_Unsigned,       StaticStr("unsigned") },
        { TokenKind_Signed,         StaticStr("signed") },
        { TokenKind_Long,           StaticStr("long") },
        { TokenKind_If,             StaticStr("if") },
        { TokenKind_Else,           StaticStr("else") },
        { TokenKind_For,            StaticStr("for") },
        { TokenKind_While,          StaticStr("while") },
        { TokenKind_Do,             StaticStr("do") },
        { TokenKind_Break,          StaticStr("break") },
        { TokenKind_Continue,       StaticStr("continue") },
    };

    usize Size = Lexer.Index - From;
    string Slice = StringView(Lexer.Code, From, Size);

    for (usize Index = 0; Index < ArrayCount(KeywordMappings); Index++)
    {
        keyword_mapping* Mapping = KeywordMappings + Index;

        if (StringEqual(Slice, Mapping->String))
        {
            Result = Mapping->Kind;
            break;
        }
    }

    return (Result);
}

local token_kind TokenizePunctuation(void)
{
    char C0 = PeekAheadCharacter(0);
    char C1 = PeekAheadCharacter(1);
    char C2 = PeekAheadCharacter(2);

    // NOTE(vak): 3 character operators
    if (Lexer.Index + 3 <= Lexer.Code.Size)
    {
        u32 Value = ((u32)C0) | ((u32)C1 << 8) | ((u32)C2 << 16);

        #define Match3(C0, C1, C2, MatchToKind) \
            else if (Value == ((u32)C0 | ((u32)C1 << 8) | ((u32)C2 << 16))) \
            { \
                Lexer.Index += 3; \
                return (MatchToKind); \
            }

        if (0) {}
        Match3('<', '<', '=', TokenKind_DoubleLessEqual)
        Match3('>', '>', '=', TokenKind_DoubleGreaterEqual)

        #undef Match3
    }

    // NOTE(vak): 2 character operators
    if (Lexer.Index + 2 <= Lexer.Code.Size)
    {
        u32 Value = ((u32)C0) | ((u32)C1 << 8);

        #define Match2(C0, C1, MatchToKind) \
            else if (Value == ((u32)C0 | ((u32)C1 << 8))) \
            { \
                Lexer.Index += 2; \
                return (MatchToKind); \
            }

        if (0) {}

        Match2('=', '=', TokenKind_DoubleEqual)
        Match2('!', '=', TokenKind_BangEqual)
        Match2('<', '=', TokenKind_LessEqual)
        Match2('>', '=', TokenKind_GreaterEqual)

        Match2('<', '<', TokenKind_DoubleLess)
        Match2('>', '>', TokenKind_DoubleGreater)

        Match2('&', '&', TokenKind_DoubleAmpersand)
        Match2('|', '|', TokenKind_DoubleBar)

        Match2('+', '=', TokenKind_PlusEqual)
        Match2('-', '=', TokenKind_MinusEqual)
        Match2('*', '=', TokenKind_StarEqual)
        Match2('/', '=', TokenKind_SlashEqual)
        Match2('%', '=', TokenKind_PercentEqual)
        Match2('&', '=', TokenKind_AmpersandEqual)
        Match2('^', '=', TokenKind_HatEqual)
        Match2('|', '=', TokenKind_BarEqual)

        Match2('+', '+', TokenKind_DoublePlus)
        Match2('-', '-', TokenKind_DoubleMinus)

        #undef Match2
    }

    // NOTE(vak): 1 character operators

    token_kind Result = (token_kind)C0;
    ConsumeCharacter();

    return (Result);
}

local void NextToken(void)
{
    token Token =
    {
        .Kind = TokenKind_EOF,
    };

    SkipWhitespace();

    if (Lexer.Index < Lexer.Code.Size)
    {
        char Character = PeekCharacter();

        usize From = Lexer.Index;

        if (IsDigit(Character))
        {
            Token.Kind = TokenizeDigit();
        }
        else if (IsIdentifierStart(Character))
        {
            Token.Kind = TokenizeIdentifier();
        }
        else if (IsPrintable(Character))
        {
            Token.Kind = TokenizePunctuation();
        }
        else
        {
            u8 Character = (u8)PeekCharacter();

            Print(Str("ERROR: Unknown character \\"));
            PrintUSize(Character);
            Print(Str(" in input."));
            PrintNewLine();
            Exit(1);
        }

        usize Size = Lexer.Index - From;

        Token.String = StringView(Lexer.Code, From, Size);
    }

    Lexer.CurrentToken = Token;
}

local usize TokenToInteger(token Token)
{
    // NOTE(vak): Assuming Token->Kind == TokenKind_Integer

    usize Result = 0;

    for (usize Index = 0; Index < Token.String.Size; Index++)
    {
        Result *= 10;
        Result += (Token.String.Data[Index] - '0');
    }

    return (Result);
}

