
// ==========================================================================================
// NOTE(vak): Compiler lexer: Responsible for converting a string into a sequence of
// tokens.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Dependencies
// ==========================================================================================

#include "shared.c"
#include "memory.c"
#include "print.c"
#include "character.c"

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

typedef enum
{
    // NOTE(vak): Punctuation characters are mapped directly to their
    // ASCII codes as token kinds. Control codes, alphabetical and
    // digit characters are free for use in representing other
    // token kinds.

    TokenKind_EOF                   = 0,
    TokenKind_Integer               = 1,
    TokenKind_Identifier            = 2,
} token_kind;

typedef u32 token_id;

#define NilTokenID (0)

typedef struct
{
    token_id First;
    u32      Count;
} token_array;

local void              SetupLexer          (void);
local void              ShutdownLexer       (void);
local token_array       Tokenize            (string Code);
local token_kind        GetTokenKind        (token_id TokenID);
local string            GetTokenString      (token_id TokenID);
local usize             GetTokenInteger     (token_id TokenID);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

typedef struct
{
    token_kind  Kind;
    u32         From;
    u32         Size;
} token;

#define DefaultTokenArenaCommited (65536  * sizeof(token))
#define DefaultTokenArenaReserved (U32Max * sizeof(token))

local arena_id  TokenArenaID        = NilArenaID;   // NOTE(vak): Storage for tokens
local string    ActiveCodeString    = NilString;    // NOTE(vak): Code string that has just been tokenized

local token* GetToken(token_id TokenID)
{
    persist token NilToken = {0};

    usize TokenCount = GetArenaUsed(TokenArenaID) / sizeof(token);

    token* Token = &NilToken;
    if ((TokenID > 0) && (TokenID <= TokenCount))
    {
        Token = (token*)GetArenaBase(TokenArenaID) + (TokenID - 1);
    }

    return (Token);
}

local void PushToken(token_kind Kind, u32 From, u32 Size)
{
    token* Token = PushArena(TokenArenaID, token);

    Token->Kind = Kind;
    Token->From = From;
    Token->Size = Size;
}

local void SetupLexer(void)
{
    TokenArenaID = CreateArena(
        DefaultTokenArenaCommited,
        DefaultTokenArenaReserved
    );
}

local void ShutdownLexer(void)
{
    DestroyArena(TokenArenaID);
    TokenArenaID = NilArenaID;
}

local usize CountWhitespaceAt(string Code, usize Index)
{
    usize Count = 0;

    while (Index + Count < Code.Size)
    {
        char Character = Code.Data[Index + Count];

        if (!IsWhitespace(Character))
            break;

        Count++;
    }

    return (Count);
}

local token TokenizeDigit(string Code, usize Index)
{
    token Token =
    {
        .Kind = TokenKind_Integer,
        .From = Index,
        .Size = 1, // NOTE(vak): We already know first character is a digit
    };

    while (Index + Token.Size < Code.Size)
    {
        char Character = Code.Data[Index + Token.Size];

        if (!IsDigit(Character))
            break;

        Token.Size++;
    }

    return (Token);
}

local token TokenizeIdentifier(string Code, usize Index)
{
    token Token =
    {
        .Kind = TokenKind_Identifier,
        .From = Index,
        .Size = 1, // NOTE(vak): We already know first character is an IdentifierStart
    };

    while (Index + Token.Size < Code.Size)
    {
        char Character = Code.Data[Index + Token.Size];

        if (!IsIdentifier(Character))
            break;

        Token.Size++;
    }

    return (Token);
}

local token TokenizePunctuation(string Code, usize Index)
{
    token Token =
    {
        .Kind = (token_kind)Code.Data[Index], // NOTE(vak): Punctuations are mapped to their ASCII codes
        .From = Index,
        .Size = 1,
    };

    return (Token);
}

local token_array Tokenize(string Code)
{
    if (Code.Size > U32Max)
    {
        Println(StdErr, Str("ERROR: code passed to tokenizer is larger than 4GB (U32Max)"));
        Exit(1);
    }

    ResetArena(TokenArenaID);
    ActiveCodeString = Code;

    token_array Result =
    {
        .First = 1, // NOTE(vak): Token arena has been reset, so first token_id is one after NilTokenID (0)
        .Count = 0,
    };

    usize Index = 0;

    while (Index < Code.Size)
    {
        Index += CountWhitespaceAt(Code, Index);

        if (Index >= Code.Size)
            break;

        token Token = {0};
        char Character = Code.Data[Index];

        if (0) {}
        else if (IsDigit(Character))            Token = TokenizeDigit(Code, Index);
        else if (IsIdentifierStart(Character))  Token = TokenizeIdentifier(Code, Index);
        else if (IsPunctuation(Character))      Token = TokenizePunctuation(Code, Index);
        else
        {
            Print(StdErr, Str("ERROR: unknown character '\\"));
            PrintUSize(StdErr, (u8)Character);
            Print(StdErr, Str("'\n"));
            Exit(1);
        }

        Index += Token.Size;

        PushToken(
            Token.Kind,
            Token.From,
            Token.Size
        );

        Result.Count++;
    }

    return (Result);
}

local token_kind GetTokenKind(token_id TokenID)
{
    token* Token = GetToken(TokenID);
    token_kind Result = Token->Kind;
    return (Result);
}

local string GetTokenString(token_id TokenID)
{
    token* Token = GetToken(TokenID);

    string Result = StringView(ActiveCodeString, Token->From, Token->Size);
    return (Result);
}

local usize GetTokenInteger(token_id TokenID)
{
    AlwaysAssert(GetTokenKind(TokenID) == TokenKind_Integer);

    usize Result = 0;

    string Digits = GetTokenString(TokenID);
    for (usize Index = 0; Index < Digits.Size; Index++)
    {
        Result *= 10;
        Result += (Digits.Data[Index] - '0');
    }

    return (Result);
}

