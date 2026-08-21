
// ==========================================================================================
// NOTE(vak): Compiler lexer: Responsible for converting a string into a sequence of
// tokens.
// ==========================================================================================

#pragma once

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

typedef struct
{
    u32 U32[1];
    u16 U16[2];
    u8  U8 [4];
} token_array_id;

#define NilTokenArrayID (token_array_id){0}
#define IsNilTokenArrayID(TokenArrayID) ((TokenArrayID).U32[0] == 0)

local token_array_id    CreateTokenArray    (void);
local void              DestroyTokenArray   (token_array_id TokenArrayID);
local void              Tokenize            (token_array_id TokenArrayID, string Code);
local u32               GetTokenCount       (token_array_id TokenArrayID);
local token_kind        GetTokenKind        (token_array_id TokenArrayID, token_id TokenID);
local string            GetTokenString      (token_array_id TokenArrayID, token_id TokenID);
local usize             GetTokenInteger     (token_array_id TokenArrayID, token_id TokenID);

// ==========================================================================================
// NOTE(vak): Example usage
// ==========================================================================================
//
//      string Code = Str("int main () { __hello + world_23_() * (10 + 10) / 120; }");
//
//      token_array_id TokenArrayID = CreateTokenArray();
//      Tokenize(TokenArrayID, Code);
//
//      u32 TokenCount = GetTokenCount(TokenArrayID);
//      for (token_id TokenID = 0; TokenID < TokenCount; TokenID++)
//      {
//          Println(StdOut, GetTokenString(TokenArrayID, TokenID));
//
//          token_kind TokenKind = GetTokenKind(TokenArrayID, TokenID);
//          if (TokenKind == '+')
//          {
//              ...
//          }
//          else if (TokenKind == TokenKind_Integer)
//          {
//              usize Value = GetTokenInteger(TokenArrayID, TokenID);
//              ...
//          }
//          ...
//      }
//

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

typedef struct
{
    token_kind  Kind;
    u32         From;
    u32         Size;
} token;

typedef struct
{
    string      Code;
    arena_id    TokenArenaID;
} token_array;

#define DefaultTokenArenaCommited (65536  * sizeof(token))
#define DefaultTokenArenaReserved (U32Max * sizeof(token))

local token_array TokenArrays[512] = {0};

local token_array* GetTokenArray(token_array_id TokenArrayID)
{
    AlwaysAssert(TokenArrayID.U32[0] > 0);
    AlwaysAssert(TokenArrayID.U32[0] <= ArrayCount(TokenArrays));

    token_array* TokenArray = TokenArrays + (TokenArrayID.U32[0] - 1);
    return (TokenArray);
}

local token* GetToken(token_array_id TokenArrayID, token_id TokenID)
{
    persist token NilToken = {0};

    token_array* TokenArray = GetTokenArray(TokenArrayID);

    token* Token = &NilToken;
    if (TokenID < GetTokenCount(TokenArrayID))
    {
        token* Base = (token*)GetArenaBase(TokenArray->TokenArenaID);
        Token = Base + TokenID;
    }

    return (Token);
}

local void PushToken(token_array_id TokenArrayID, token_kind Kind, u32 From, u32 Size)
{
    token_array* TokenArray = GetTokenArray(TokenArrayID);

    token* Token = PushArena(TokenArray->TokenArenaID, token);

    Token->Kind = Kind;
    Token->From = From;
    Token->Size = Size;
}

local token_array_id FindFreeTokenArraySlot(void)
{
    token_array_id TokenArrayID = NilTokenArrayID;

    for (u32 Index = 0; Index < ArrayCount(TokenArrays); Index++)
    {
        token_array* TokenArray = TokenArrays + Index;

        if (IsNilArenaID(TokenArray->TokenArenaID))
        {
            TokenArrayID.U32[0] = 1 + Index;
            break;
        }
    }

    return (TokenArrayID);
}

local token_array_id CreateTokenArray(void)
{
    token_array_id TokenArrayID = FindFreeTokenArraySlot();
    AlwaysAssert(!IsNilTokenArrayID(TokenArrayID));

    token_array* TokenArray = GetTokenArray(TokenArrayID);

    TokenArray->Code = NilString;

    TokenArray->TokenArenaID = CreateArena(
        DefaultTokenArenaCommited,
        DefaultTokenArenaReserved
    );

    if (IsNilArenaID(TokenArray->TokenArenaID))
    {
        Println(StdErr, Str("ERROR: failed to create token arena for token array"));
        Exit(1);
    }

    return (TokenArrayID);
}

local void DestroyTokenArray(token_array_id TokenArrayID)
{
    token_array* TokenArray = GetTokenArray(TokenArrayID);
    DestroyArena(TokenArray->TokenArenaID);
    ZeroType(TokenArray);
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

local void Tokenize(token_array_id TokenArrayID, string Code)
{
    token_array* TokenArray = GetTokenArray(TokenArrayID);

    ResetArena(TokenArray->TokenArenaID);
    TokenArray->Code = Code;

    if (Code.Size > U32Max)
    {
        Println(StdErr, Str("ERROR: Code passed to compiler lexer is larger than 4GB (U32Max)"));
        Exit(1);
    }

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
            TokenArrayID,
            Token.Kind,
            Token.From,
            Token.Size
        );
    }
}

local u32 GetTokenCount(token_array_id TokenArrayID)
{
    token_array* TokenArray = GetTokenArray(TokenArrayID);

    u32 Result = (u32)(GetArenaUsed(TokenArray->TokenArenaID) / sizeof(token));
    return (Result);
}

local token_kind GetTokenKind(token_array_id TokenArrayID, token_id TokenID)
{
    token* Token = GetToken(TokenArrayID, TokenID);
    token_kind Result = Token->Kind;
    return (Result);
}

local string GetTokenString(token_array_id TokenArrayID, token_id TokenID)
{
    token_array* TokenArray = GetTokenArray(TokenArrayID);
    token* Token = GetToken(TokenArrayID, TokenID);

    string Result = StringView(TokenArray->Code, Token->From, Token->Size);
    return (Result);
}

local usize GetTokenInteger(token_array_id TokenArrayID, token_id TokenID)
{
    token_array* TokenArray = GetTokenArray(TokenArrayID);

    AlwaysAssert(GetTokenKind(TokenArrayID, TokenID) == TokenKind_Integer);

    usize Result = 0;

    string Digits = GetTokenString(TokenArrayID, TokenID);
    for (usize Index = 0; Index < Digits.Size; Index++)
    {
        Result *= 10;
        Result += (Digits.Data[Index] - '0');
    }

    return (Result);
}

