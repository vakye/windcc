
// ==========================================================================================
// NOTE(vak): Compiler parser: Converts a sequence of tokens into a syntax tree composed of
// operations. This syntax tree is then fed into the code generator for conversion into
// machine code.
// ==========================================================================================

#pragma once

// ==========================================================================================
// NOTE(vak): Interface
// ==========================================================================================

// TODO(vak): Properly document node kinds and their usage of the members of the node
// structure. It is an absolute mess right now...

typedef enum
{
    NodeKind_Nil = 0,

    NodeKind_Block,

    NodeKind_Integer,
    NodeKind_Identifier,

    NodeKind_Negate,
    NodeKind_LogicalNot,
    NodeKind_BitwiseNot,

    NodeKind_Add,
    NodeKind_Sub,
    NodeKind_Mul,
    NodeKind_Div,
    NodeKind_Mod,

    NodeKind_Equal,
    NodeKind_NotEqual,
    NodeKind_Less,
    NodeKind_Greater,
    NodeKind_LessEqual,
    NodeKind_GreaterEqual,

    NodeKind_ShiftLeft,
    NodeKind_ShiftRight,

    NodeKind_BitwiseAnd,
    NodeKind_BitwiseXor,
    NodeKind_BitwiseOr,

    NodeKind_LogicalAnd,
    NodeKind_LogicalOr,

    NodeKind_Ternary,

    NodeKind_PostIncrement,
    NodeKind_PostDecrement,
    NodeKind_PreIncrement,
    NodeKind_PreDecrement,

    NodeKind_AddressOf,
    NodeKind_Dereference,

    NodeKind_Assign,
    NodeKind_Declare,
    NodeKind_If,
    NodeKind_For,
    NodeKind_Break,
    NodeKind_Continue,
} node_kind;

typedef enum
{
    TypeSpecKind_Normal = 0,
    TypeSpecKind_Function,
} type_spec_kind;

typedef struct type_spec type_spec;
struct type_spec
{
    type_spec_kind Kind;
    b32 SignDoesntMatter;
    b32 Signed;
    usize Bytes;
    type_spec* PointingTo;
    type_spec* ReturnType;
    usize ArrayCount;
};

typedef struct node node;
struct node
{
    node_kind Kind;
    node* Next;
    token Token;

    usize Integer;

    type_spec TypeSpec;
    string Identifier;
    node* Initializer;
    node* FunctionBody;

    node* Left;
    node* Right;

    node* IfCond;
    node* IfThen;
    node* IfElse;

    node* FirstStatement;

    node* ForInit;
    node* ForCond;
    node* ForIter;
    node* ForBody;
};

// NOTE(vak): The sequence of tokens the parser receives can be set with EquipLexerCode(Code).
// Example usage:
//
//      string Code = Str("10 + 10");
//      EquipLexerCode(Code);
//      node* Node = Parse();
//
// will parse the code string "10 + 10" into a NodeKind_Add composed of two NodeKind_Integer

// TODO(vak): Make it so that the Parse() function can perhaps receive a lexer_id handle, so
// as to support multiple token streams instead of being confined to just using EquipLexerCode(),
// which is just a single token stream.

local node* Parse(void); 

// NOTE(vak): More primitive parsing functions

local node* ParseExpression(void);
local node* ParseStatement(void);
local node* ParseBlock(void);

// ==========================================================================================
// NOTE(vak): Implementation
// ==========================================================================================

local arena_id NodeArenaID = NilArenaID; // NOTE(vak): Array of node
local arena_id TypeArenaID = NilArenaID; // NOTE(vak): Array of type_spec

local node* AllocateNode(node_kind Kind, token Token)
{
    if (IsNilArenaID(NodeArenaID))
        NodeArenaID = CreateArena(MB(1), GB(16));

    node* Node = PushArena(NodeArenaID, node);

    ZeroType(Node);
    Node->Kind = Kind;
    Node->Token = Token;

    return (Node);
}

local node* AllocateUnaryNode(node_kind Kind, token Token, node* Left)
{
    node* Node = AllocateNode(Kind, Token);
    Node->Left = Left;

    return (Node);
}

local node* AllocateBinaryNode(node_kind Kind, token Token, node* Left, node* Right)
{
    node* Node = AllocateNode(Kind, Token);

    Node->Left = Left;
    Node->Right = Right;

    return (Node);
}

local type_spec* AllocateTypeSpec(type_spec_kind Kind)
{
    if (IsNilArenaID(TypeArenaID))
        TypeArenaID = CreateArena(MB(16), GB(16));

    type_spec* TypeSpec = PushArena(TypeArenaID, type_spec);

    ZeroType(TypeSpec);
    TypeSpec->Kind = Kind;

    return (TypeSpec);
}

local node* ParsePrimary(void)
{
    node* Node = 0;

    token Token = GetCurrentToken();

    if (NextIfMatchToken(TokenKind_Integer))
    {
        Node = AllocateNode(NodeKind_Integer, Token);
        Node->Integer = TokenToInteger(Token);
    }
    else if (NextIfMatchToken(TokenKind_Identifier))
    {
        Node = AllocateNode(NodeKind_Identifier, Token);
        Node->Identifier = Token.String;
    }
    else if (NextIfMatchToken('('))
    {
        Node = ParseExpression();

        if (!NextIfMatchToken(')'))
        {
            Println(Str("ERROR: expected matching ')'"));
            Exit(1);
        }
    }
    else if (MatchToken(';'))
    {
        // NOTE(vak): Ignore
    }
    else
    {
        Println(Str("ERROR: syntax error"));
        Exit(1);
    }

    return (Node);
}

local node* ParsePostfix(void)
{
    node* Node = ParsePrimary();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken(TokenKind_DoublePlus))
        {
            Node = AllocateUnaryNode(NodeKind_PostIncrement, Token, Node);
        }
        else if (NextIfMatchToken(TokenKind_DoubleMinus))
        {
            Node = AllocateUnaryNode(NodeKind_PostDecrement, Token, Node);
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParsePrefix(void)
{
    node* Node = 0;

    token Token = GetCurrentToken();

    if (NextIfMatchToken('-'))
    {
        Node = AllocateUnaryNode(NodeKind_Negate, Token, ParsePrefix());
    }
    else if (NextIfMatchToken('!'))
    {
        Node = AllocateUnaryNode(NodeKind_LogicalNot, Token, ParsePrefix());
    }
    else if (NextIfMatchToken('~'))
    {
        Node = AllocateUnaryNode(NodeKind_BitwiseNot, Token, ParsePrefix());
    }
    else if (NextIfMatchToken('+'))
    {
        Node = ParsePrefix();
    }
    else if (NextIfMatchToken(TokenKind_DoublePlus))
    {
        Node = AllocateUnaryNode(NodeKind_PreIncrement, Token, ParsePrefix());
    }
    else if (NextIfMatchToken(TokenKind_DoubleMinus))
    {
        Node = AllocateUnaryNode(NodeKind_PreDecrement, Token, ParsePrefix());
    }
    else if (NextIfMatchToken('&'))
    {
        Node = AllocateUnaryNode(NodeKind_AddressOf, Token, ParsePrefix());
    }
    else if (NextIfMatchToken('*'))
    {
        Node = AllocateUnaryNode(NodeKind_Dereference, Token, ParsePrefix());
    }
    else
    {
        Node = ParsePostfix();
    }

    return (Node);
}

local node* ParseFactor(void)
{
    node* Node = ParsePrefix();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('*'))
        {
            Node = AllocateBinaryNode(NodeKind_Mul, Token, Node, ParsePrefix());
        }
        else if (NextIfMatchToken('/'))
        {
            Node = AllocateBinaryNode(NodeKind_Div, Token, Node, ParsePrefix());
        }
        else if (NextIfMatchToken('%'))
        {
            Node = AllocateBinaryNode(NodeKind_Mod, Token, Node, ParsePrefix());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseSum(void)
{
    node* Node = ParseFactor();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('+'))
        {
            Node = AllocateBinaryNode(NodeKind_Add, Token, Node, ParseFactor());
        }
        else if (NextIfMatchToken('-'))
        {
            Node = AllocateBinaryNode(NodeKind_Sub, Token, Node, ParseFactor());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseShift(void)
{
    node* Node = ParseSum();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken(TokenKind_DoubleLess))
        {
            Node = AllocateBinaryNode(NodeKind_ShiftLeft, Token, Node, ParseSum());
        }
        else if (NextIfMatchToken(TokenKind_DoubleGreater))
        {
            Node = AllocateBinaryNode(NodeKind_ShiftRight, Token, Node, ParseSum());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseComparison(void)
{
    node* Node = ParseShift();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('<'))
        {
            Node = AllocateBinaryNode(NodeKind_Less, Token, Node, ParseShift());
        }
        else if (NextIfMatchToken('>'))
        {
            Node = AllocateBinaryNode(NodeKind_Greater, Token, Node, ParseShift());
        }
        else if (NextIfMatchToken(TokenKind_LessEqual))
        {
            Node = AllocateBinaryNode(NodeKind_LessEqual, Token, Node, ParseShift());
        }
        else if (NextIfMatchToken(TokenKind_GreaterEqual))
        {
            Node = AllocateBinaryNode(NodeKind_GreaterEqual, Token, Node, ParseShift());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseEquality(void)
{
    node* Node = ParseComparison();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken(TokenKind_DoubleEqual))
        {
            Node = AllocateBinaryNode(NodeKind_Equal, Token, Node, ParseComparison());
        }
        else if (NextIfMatchToken(TokenKind_BangEqual))
        {
            Node = AllocateBinaryNode(NodeKind_NotEqual, Token, Node, ParseComparison());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseAnd(void)
{
    node* Node = ParseEquality();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('&'))
        {
            Node = AllocateBinaryNode(NodeKind_BitwiseAnd, Token, Node, ParseEquality());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseXor(void)
{
    node* Node = ParseAnd();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('^'))
        {
            Node = AllocateBinaryNode(NodeKind_BitwiseXor, Token, Node, ParseAnd());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseOr(void)
{
    node* Node = ParseXor();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken('|'))
        {
            Node = AllocateBinaryNode(NodeKind_BitwiseOr, Token, Node, ParseXor());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseLogicalAnd(void)
{
    node* Node = ParseOr();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken(TokenKind_DoubleAmpersand))
        {
            Node = AllocateBinaryNode(NodeKind_LogicalAnd, Token, Node, ParseOr());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseLogicalOr(void)
{
    node* Node = ParseLogicalAnd();

    for (;;)
    {
        token Token = GetCurrentToken();

        if (NextIfMatchToken(TokenKind_DoubleBar))
        {
            Node = AllocateBinaryNode(NodeKind_LogicalOr, Token, Node, ParseLogicalAnd());
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseTernary(void)
{
    node* Node = ParseLogicalOr();

    token Token = GetCurrentToken();

    if (NextIfMatchToken('?'))
    {
        node* TernaryNode = AllocateNode(NodeKind_Ternary, Token);

        TernaryNode->IfCond = Node;
        TernaryNode->IfThen = ParseTernary();

        if (!NextIfMatchToken(':'))
        {
            Println(Str("ERROR: missing ':' in ternary expression"));
            Exit(1);
        }

        TernaryNode->IfElse = ParseTernary();

        Node = TernaryNode;
    }

    return (Node);
}

local node* ParseAssignment(void)
{
    node* Node = ParseTernary();

    token Token = GetCurrentToken();

    if (NextIfMatchToken('='))
    {
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, ParseAssignment());
    }
    else if (NextIfMatchToken(TokenKind_PlusEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_Add, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_MinusEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_Sub, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_StarEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_Mul, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_SlashEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_Div, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_PercentEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_Mod, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_DoubleLessEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_ShiftLeft, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_DoubleGreaterEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_ShiftRight, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_AmpersandEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_BitwiseAnd, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_HatEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_BitwiseXor, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (NextIfMatchToken(TokenKind_BarEqual))
    {
        node* OpNode = AllocateBinaryNode(NodeKind_BitwiseOr, Token, Node, ParseAssignment());
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }

    return (Node);
}

local node* ParseExpression(void)
{
    node* Node = ParseAssignment();
    return (Node);
}

local node* ParseDeclaration(type_spec TypeSpec)
{
    node* Node = AllocateNode(NodeKind_Declare, GetCurrentToken());

    Node->TypeSpec = TypeSpec;

    while (NextIfMatchToken('*'))
    {
        type_spec* PointingTo = AllocateTypeSpec(TypeSpecKind_Normal);
        *PointingTo = Node->TypeSpec;

        Node->TypeSpec.Bytes = 8;
        Node->TypeSpec.PointingTo = PointingTo;
    }

    token IdentifierToken = GetCurrentToken();

    if (!NextIfMatchToken(TokenKind_Identifier))
    {
        Print(Str("ERROR: '"));
        Print(IdentifierToken.String);
        Print(Str("' is not a valid variable name"));
        PrintNewLine();
        Exit(1);
    }

    Node->Identifier = IdentifierToken.String;

    while (NextIfMatchToken('['))
    {
        token Token = GetCurrentToken();

        if (!MatchToken(TokenKind_Integer))
        {
            Println(Str("ERROR: expected array size to be specified"));
            Exit(1);
        }

        type_spec* PointingTo = AllocateTypeSpec(TypeSpecKind_Normal);
        *PointingTo = Node->TypeSpec;

        Node->TypeSpec.Bytes = 8;
        Node->TypeSpec.PointingTo = PointingTo;
        Node->TypeSpec.ArrayCount = TokenToInteger(Token);

        if (Node->TypeSpec.ArrayCount == 0)
        {
            Println(Str("ERROR: specified array size must be larger than 0"));
            Exit(1);
        }

        NextToken();

        if (!NextIfMatchToken(']'))
        {
            Println(Str("ERROR: missing matching ']'"));
            Exit(1);
        }
    }

    token Token = GetCurrentToken();

    if (NextIfMatchToken('='))
    {
        node* IdentifierNode = AllocateNode(NodeKind_Identifier, IdentifierToken);
        IdentifierNode->Identifier = Node->Identifier;

        Node->Initializer = AllocateBinaryNode(NodeKind_Assign, Token, IdentifierNode, ParseExpression());

        if (!NextIfMatchToken(';'))
        {
            Println(Str("ERROR: expected ';' at end of statement"));
            Exit(1);
        }
    }
    else if (NextIfMatchToken('('))
    {
        type_spec* ReturnType = AllocateTypeSpec(TypeSpecKind_Normal);
        *ReturnType = Node->TypeSpec;

        Node->TypeSpec.Kind = TypeSpecKind_Function;
        Node->TypeSpec.ReturnType = ReturnType;

        if (!NextIfMatchToken(')'))
        {
            Println(Str("ERROR: expected matching ')'"));
            Exit(1);
        }

        if (NextIfMatchToken(';'))
        {
        }
        else if (MatchToken('{'))
        {
            Node->FunctionBody = ParseBlock();
        }
        else
        {
            Println(Str("ERROR: syntax error"));
            Exit(1);
        }
    }
    else if (NextIfMatchToken(';'))
    {
    }
    else
    {
        Println(Str("ERROR: syntax error"));
        Exit(1);
    }

    return (Node);
}

local type_spec ParseDeclarationSpecifiers(void)
{
    u32 UnsignedCount = 0;
    u32 SignedCount = 0;
    u32 IntCount = 0;
    u32 ShortCount = 0;
    u32 CharCount = 0;
    u32 LongCount = 0;

    // NOTE(vak): Count qualifiers

    for (;;)
    {
        if (0) {}
        else if (NextIfMatchToken(TokenKind_Unsigned))      UnsignedCount++;
        else if (NextIfMatchToken(TokenKind_Signed))        SignedCount++;
        else if (NextIfMatchToken(TokenKind_Int))           IntCount++;
        else if (NextIfMatchToken(TokenKind_Short))         ShortCount++;
        else if (NextIfMatchToken(TokenKind_Char))          CharCount++;
        else if (NextIfMatchToken(TokenKind_Long))          LongCount++;
        else break;
    }

    // NOTE(vak): Count check
    {
        b32 HasError = true;

        if (0) {}
        else if (UnsignedCount > 1)     Println(Str("ERROR: too many 'unsigned' in declaration"));
        else if (SignedCount > 1)       Println(Str("ERROR: too many 'signed' in declaration"));
        else if (IntCount > 1)          Println(Str("ERROR: too many 'int' in declaration"));
        else if (ShortCount > 1)        Println(Str("ERROR: too many 'short' in declaration"));
        else if (CharCount > 1)         Println(Str("ERROR: too many 'char' in declaration"));
        else if (LongCount > 2)         Println(Str("ERROR: too many 'long' in declaration"));
        else HasError = false;

        if (HasError)
            Exit(1);
    }

    // NOTE(vak): Qualifier checks
    {
        b32 HasError = true;

        b32 NoTypeSpecified =
            (UnsignedCount || SignedCount) &&
            !(IntCount || CharCount || ShortCount || LongCount);

        b32 ConflictingTypes =
            (CharCount && ShortCount) ||
            (CharCount && LongCount) ||
            (ShortCount && CharCount);

        b32 ConflictingSign =
            (UnsignedCount && SignedCount);

        if (0) {}
        else if (NoTypeSpecified)       Println(Str("ERROR: no type qualifier in declaration"));
        else if (ConflictingTypes)      Println(Str("ERROR: conflicting type qualifiers in declaration"));
        else if (ConflictingSign)       Println(Str("ERROR: conflicting sign qualifier in declaration"));
        else
            HasError = false;

        if (HasError)
            Exit(1);
    }

    type_spec TypeSpec = {0};

    // NOTE(vak): Construct type spec
    {
        if (UnsignedCount)          TypeSpec.Signed = false;
        else                        TypeSpec.Signed = true;

        if (0) {}
        else if (LongCount  == 2)   TypeSpec.Bytes = 8;
        else if (LongCount  == 1)   TypeSpec.Bytes = 4;
        else if (ShortCount == 1)   TypeSpec.Bytes = 2;
        else if (CharCount  == 1)   TypeSpec.Bytes = 1;
        else if (IntCount   == 1)   TypeSpec.Bytes = 4; // NOTE(vak): "int" case should be put last to prevent conflict
    }

    return (TypeSpec);
}

local node* ParseBlock(void)
{
    token Token = GetCurrentToken();

    if (!NextIfMatchToken('{'))
    {
        Println(Str("ERROR: expected '{' at start of block"));
        Exit(1);
    }

    node* BlockNode = AllocateNode(NodeKind_Block, Token);

    node* FirstStatement = 0;
    node* LastStatement = 0;

    for (;;)
    {
        if (MatchToken(TokenKind_EOF) || MatchToken('}'))
            break;

        node* Statement = ParseStatement();
        if (!Statement)
            continue;

        if (!FirstStatement)
        {
            FirstStatement = Statement;
            LastStatement = Statement;
        }
        else
        {
            LastStatement->Next = Statement;
            LastStatement = Statement;
        }
    }

    if (!NextIfMatchToken('}'))
    {
        Println(Str("ERROR: missing '}' at end of block"));
        Exit(1);
    }

    BlockNode->FirstStatement = FirstStatement;

    return (BlockNode);
}

local node* ParseStatement(void)
{
    node* Node = 0;

    token Token = GetCurrentToken();

    type_spec TypeSpec = ParseDeclarationSpecifiers();
    if (TypeSpec.Bytes)
    {
        Node = ParseDeclaration(TypeSpec);

    }
    else if (NextIfMatchToken(TokenKind_If))
    {
        Node = AllocateNode(NodeKind_If, Token);

        if (!MatchToken('('))
        {
            Println(Str("ERROR: expected if conditional to start with '('"));
            Exit(1);
        }

        Node->IfCond = ParseExpression();

        Node->IfThen = ParseStatement();

        if (NextIfMatchToken(TokenKind_Else))
        {
            Node->IfElse = ParseStatement();
        }
    }
    else if (NextIfMatchToken(TokenKind_For))
    {
        Node = AllocateNode(NodeKind_For, Token);

        if (!NextIfMatchToken('('))
        {
            Println(Str("ERROR: expected '(' after for"));
            Exit(1);
        }

        // NOTE(vak): For loop init clause can either be a declaration or
        // an expression

        type_spec TypeSpec = ParseDeclarationSpecifiers();

        if (TypeSpec.Bytes)
            Node->ForInit = ParseDeclaration(TypeSpec);
        else
        {
            Node->ForInit = ParseExpression();

            if (!NextIfMatchToken(';'))
            {
                Println(Str("ERROR: expected ';' after for loop initializer"));
                Exit(1);
            }
        }

        // NOTE(vak): For loop condition

        Node->ForCond = ParseExpression();

        if (!NextIfMatchToken(';'))
        {
            Println(Str("ERROR: expected ';' after for loop conditional"));
            Exit(1);
        }

        // NOTE(vak): For loop iteration update

        Node->ForIter = ParseExpression();

        if (!NextIfMatchToken(')'))
        {
            Println(Str("ERROR: missing ')' in for loop"));
            Exit(1);
        }

        Node->ForBody = ParseStatement();
    }
    else if (NextIfMatchToken(TokenKind_While))
    {
        Node = AllocateNode(NodeKind_For, Token);

        if (!MatchToken('('))
        {
            Println(Str("ERROR: expected '(' after while"));
            Exit(1);
        }

        Node->ForCond = ParseExpression();
        Node->ForBody = ParseStatement();
    }
    else if (NextIfMatchToken(TokenKind_Do))
    {
        Node = AllocateNode(NodeKind_For, Token);

        Node->ForBody = ParseStatement();
        Node->ForInit = Node->ForBody;

        if (!NextIfMatchToken(TokenKind_While))
        {
            Println(Str("ERROR: missing 'while' in do-while loop"));
            Exit(1);
        }

        if (!MatchToken('('))
        {
            Println(Str("ERROR: expected '(' after while"));
            Exit(1);
        }

        Node->ForCond = ParseExpression();

        if (!NextIfMatchToken(';'))
        {
            Println(Str("ERROR: expected ';' at end of do-while loop"));
            Exit(1);
        }
    }
    else if (NextIfMatchToken(TokenKind_Break))
    {
        Node = AllocateNode(NodeKind_Break, Token);
    }
    else if (NextIfMatchToken(TokenKind_Continue))
    {
        Node = AllocateNode(NodeKind_Continue, Token);
    }
    else if (MatchToken('{'))
    {
        Node = ParseBlock();
    }
    else
    {
        Node = ParseExpression();

        if (!NextIfMatchToken(';'))
        {
            Println(Str("ERROR: expected ';' at end of statement"));
            Exit(1);
        }
    }

    return (Node);
}

local node* Parse(void)
{
    node* First = 0;
    node* Last = 0;

    while (!MatchToken(TokenKind_EOF))
    {
        node* Statement = ParseStatement();
        if (!Statement)
            continue;

        if (!First)
        {
            First = Statement;
            Last = Statement;
        }
        else
        {
            Last->Next = Statement;
            Last = Statement;
        }
    }

    return (First);
}

