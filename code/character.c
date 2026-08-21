
// ==========================================================================================
// NOTE(vak): Character classification
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

local b32 IsWhitespace          (char Character); // NOTE(vak): ' ' | '\t' | '\n' | '\r'
local b32 IsDigit               (char Character); // NOTE(vak): '0' .. '9'
local b32 IsLowercase           (char Character); // NTOE(vak): 'a' .. 'z'
local b32 IsUppercase           (char Character); // NOTE(vak): 'A' .. 'Z'
local b32 IsAlphabet            (char Character); // NOTE(vak): Lowercase | Uppercase
local b32 IsIdentifierStart     (char Character); // NOTE(vak): '_' | Alphabet
local b32 IsIdentifier          (char Character); // NOTE(vak0: '_' | Alphabet | Digit
local b32 IsPunctuation         (char Character); // NOTE(vak): All punctuation except '_'

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local b32 IsWhitespace(char Character)
{
    b32 Result =
        (Character == ' ') ||
        (Character == '\t') ||
        (Character == '\r') ||
        (Character == '\n');

    return (Result);
}

local b32 IsDigit(char Character)
{
    b32 Result = (Character >= '0') && (Character <= '9');
    return (Result);
}

local b32 IsLowercase(char Character)
{
    b32 Result = (Character >= 'a') && (Character <= 'z');
    return (Result);
}

local b32 IsUppercase(char Character)
{
    b32 Result = (Character >= 'A') && (Character <= 'Z');
    return (Result);
}

local b32 IsAlphabet(char Character)
{
    b32 Result = IsLowercase(Character) || IsUppercase(Character);
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

local b32 IsPunctuation(char Character)
{
    // NOTE(vak): '_' (ASCII code 95) is excluded here since
    // IsIdentifierStart already classifies it as an identifier.

    b32 Result =
        ((Character >=  33) && (Character <=  47)) ||
        ((Character >=  58) && (Character <=  63)) ||
        ((Character >=  91) && (Character <=  94)) ||
        ((Character >=  96) && (Character <=  96)) ||
        ((Character >= 123) && (Character <= 126));

    return (Result);
}

