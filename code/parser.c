
#pragma once

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

    NodeKind_Assign,
    NodeKind_Declare,
    NodeKind_If,
    NodeKind_For,
} node_kind;

typedef struct
{
    b32 Signed;
    usize Bytes;
} type_spec;

typedef struct node node;
struct node
{
    node_kind Kind;
    node* Next;
    token* Token;

    usize Integer;

    type_spec TypeSpec;
    string Identifier;
    node* Initializer;

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

local node* AllocateNode(node_kind Kind, token* Token)
{
    node* Node = Allocate(sizeof(node));

    ZeroType(Node);
    Node->Kind = Kind;
    Node->Token = Token;

    return (Node);
}

local node* AllocateUnaryNode(node_kind Kind, token* Token, node* Left)
{
    node* Node = AllocateNode(Kind, Token);
    Node->Left = Left;

    return (Node);
}

local node* AllocateBinaryNode(node_kind Kind, token* Token, node* Left, node* Right)
{
    node* Node = AllocateNode(Kind, Token);

    Node->Left = Left;
    Node->Right = Right;

    return (Node);
}

local node* ParseExpression(token** ParseAt);

local node* ParsePrimary(token** ParseAt)
{
    node* Node = 0;

    token* Token = *ParseAt;
    if (Token->Kind == TokenKind_Integer)
    {
        Node = AllocateNode(NodeKind_Integer, Token);
        Node->Integer = TokenToInteger(Token);
        *ParseAt = Token->Next;
    }
    else if (Token->Kind == TokenKind_Identifier)
    {
        Node = AllocateNode(NodeKind_Identifier, Token);
        Node->Identifier = Token->String;
        *ParseAt = Token->Next;
    }
    else if (Token->Kind == '(')
    {
        *ParseAt = Token->Next;

        Node = ParseExpression(ParseAt);

        Token = *ParseAt;
        if (Token->Kind != ')')
        {
            Println(Str("ERROR: expected matching ')'"));
            Exit(1);
        }

        *ParseAt = Token->Next;
    }
    else if (Token->Kind == ';')
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

local node* ParsePostfix(token** ParseAt)
{
    node* Node = ParsePrimary(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == TokenKind_DoublePlus)
        {
            *ParseAt = Token->Next;
            Node = AllocateUnaryNode(NodeKind_PostIncrement, Token, Node);
        }
        else if (Token->Kind == TokenKind_DoubleMinus)
        {
            *ParseAt = Token->Next;
            Node = AllocateUnaryNode(NodeKind_PostDecrement, Token, Node);
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParsePrefix(token** ParseAt)
{
    node* Node = 0;

    token* Token = *ParseAt;
    if (Token->Kind == '-')
    {
        *ParseAt = Token->Next;
        Node = AllocateUnaryNode(NodeKind_Negate, Token, ParsePrefix(ParseAt));
    }
    else if (Token->Kind == '!')
    {
        *ParseAt = Token->Next;
        Node = AllocateUnaryNode(NodeKind_LogicalNot, Token, ParsePrefix(ParseAt));
    }
    else if (Token->Kind == '~')
    {
        *ParseAt = Token->Next;
        Node = AllocateUnaryNode(NodeKind_BitwiseNot, Token, ParsePrefix(ParseAt));
    }
    else if (Token->Kind == '+')
    {
        *ParseAt = Token->Next;
        Node = ParsePrefix(ParseAt);
    }
    else if (Token->Kind == TokenKind_DoublePlus)
    {
        *ParseAt = Token->Next;
        Node = AllocateUnaryNode(NodeKind_PreIncrement, Token, ParsePrefix(ParseAt));
    }
    else if (Token->Kind == TokenKind_DoubleMinus)
    {
        *ParseAt = Token->Next;
        Node = AllocateUnaryNode(NodeKind_PreDecrement, Token, ParsePrefix(ParseAt));
    }
    else
    {
        Node = ParsePostfix(ParseAt);
    }

    return (Node);
}

local node* ParseFactor(token** ParseAt)
{
    node* Node = ParsePrefix(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == '*')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Mul, Token, Node, ParsePrefix(ParseAt));
        }
        else if (Token->Kind == '/')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Div, Token, Node, ParsePrefix(ParseAt));
        }
        else if (Token->Kind == '%')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Mod, Token, Node, ParsePrefix(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseSum(token** ParseAt)
{
    node* Node = ParseFactor(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == '+')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Add, Token, Node, ParseFactor(ParseAt));
        }
        else if (Token->Kind == '-')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Sub, Token, Node, ParseFactor(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseShift(token** ParseAt)
{
    node* Node = ParseSum(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == TokenKind_DoubleLess)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_ShiftLeft, Token, Node, ParseSum(ParseAt));
        }
        else if (Token->Kind == TokenKind_DoubleGreater)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_ShiftRight, Token, Node, ParseSum(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseComparison(token** ParseAt)
{
    node* Node = ParseShift(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == '<')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Less, Token, Node, ParseShift(ParseAt));
        }
        else if (Token->Kind == '>')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Greater, Token, Node, ParseShift(ParseAt));
        }
        else if (Token->Kind == TokenKind_LessEqual)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_LessEqual, Token, Node, ParseShift(ParseAt));
        }
        else if (Token->Kind == TokenKind_GreaterEqual)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_GreaterEqual, Token, Node, ParseShift(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseEquality(token** ParseAt)
{
    node* Node = ParseComparison(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == TokenKind_DoubleEqual)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_Equal, Token, Node, ParseComparison(ParseAt));
        }
        else if (Token->Kind == TokenKind_BangEqual)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_NotEqual, Token, Node, ParseComparison(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseAnd(token** ParseAt)
{
    node* Node = ParseEquality(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == '&')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_BitwiseAnd, Token, Node, ParseEquality(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseXor(token** ParseAt)
{
    node* Node = ParseAnd(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == '^')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_BitwiseXor, Token, Node, ParseAnd(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseOr(token** ParseAt)
{
    node* Node = ParseXor(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == '|')
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_BitwiseOr, Token, Node, ParseXor(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseLogicalAnd(token** ParseAt)
{
    node* Node = ParseOr(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == TokenKind_DoubleAmpersand)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_LogicalAnd, Token, Node, ParseOr(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseLogicalOr(token** ParseAt)
{
    node* Node = ParseLogicalAnd(ParseAt);

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == TokenKind_DoubleBar)
        {
            *ParseAt = Token->Next;
            Node = AllocateBinaryNode(NodeKind_LogicalOr, Token, Node, ParseLogicalAnd(ParseAt));
        }
        else
        {
            break;
        }
    }

    return (Node);
}

local node* ParseTernary(token** ParseAt)
{
    node* Node = ParseLogicalOr(ParseAt);

    token* Token = *ParseAt;
    if (Token->Kind == '?')
    {
        node* TernaryNode = AllocateNode(NodeKind_Ternary, Token);

        *ParseAt = Token->Next;

        TernaryNode->IfCond = Node;
        TernaryNode->IfThen = ParseTernary(ParseAt);

        Token = *ParseAt;
        if (Token->Kind != ':')
        {
            Println(Str("ERROR: missing ':' in ternary expression"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        TernaryNode->IfElse = ParseTernary(ParseAt);

        Node = TernaryNode;
    }

    return (Node);
}

local node* ParseAssignment(token** ParseAt)
{
    node* Node = ParseTernary(ParseAt);

    token* Token = *ParseAt;
    if (Token->Kind == '=')
    {
        *ParseAt = Token->Next;
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, ParseAssignment(ParseAt));
    }
    else if (Token->Kind == TokenKind_PlusEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_Add, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_MinusEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_Sub, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_StarEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_Mul, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_SlashEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_Div, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_PercentEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_Mod, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_DoubleLessEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_ShiftLeft, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_DoubleGreaterEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_ShiftRight, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_AmpersandEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_BitwiseAnd, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_HatEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_BitwiseXor, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }
    else if (Token->Kind == TokenKind_BarEqual)
    {
        *ParseAt = Token->Next;
        node* OpNode = AllocateBinaryNode(NodeKind_BitwiseOr, Token, Node, ParseAssignment(ParseAt));
        Node = AllocateBinaryNode(NodeKind_Assign, Token, Node, OpNode);
    }

    return (Node);
}

local node* ParseExpression(token** ParseAt)
{
    node* Node = ParseAssignment(ParseAt);
    return (Node);
}

local node* ParseDeclaration(token** ParseAt, type_spec TypeSpec)
{
    token* Token = *ParseAt;

    node* Node = AllocateNode(NodeKind_Declare, Token);

    if (Token->Kind != TokenKind_Identifier)
    {
        Print(Str("ERROR: '"));
        Print(Token->String);
        Print(Str("' is not a valid variable name"));
        PrintNewLine();
        Exit(1);
    }

    Node->TypeSpec = TypeSpec;
    Node->Identifier = Token->String;

    // NOTE(vak): No need to advance past identifier since ParseExpression
    // can use it to generate an assignment node.

    Node->Initializer = ParseExpression(ParseAt);

    return (Node);
}

local type_spec ParseDeclarationSpecifiers(token** ParseAt)
{
    type_spec TypeSpec = {0};
    TypeSpec.Signed = true;

    b32 DetectedUnsigned = false;
    b32 DetectedSigned = false;
    b32 DetectedInt = false;
    b32 DetectedShort = false;
    b32 DetectedChar = false;
    b32 DetectedLong = false;
    b32 DetectedLongLong = false;

    for (;;)
    {
        token* Token = *ParseAt;

        if (Token->Kind == TokenKind_Unsigned)
        {
            if (DetectedUnsigned)
            {
                Println(Str("ERROR: multiple 'unsigned' specifiers"));
                Exit(1);
            }

            if (DetectedSigned)
            {
                Println(Str("ERROR: 'signed' and 'unsigned' specifiers used together"));
                Exit(1);
            }

            TypeSpec.Signed = false;
            DetectedUnsigned = true;

            *ParseAt = Token->Next;
        }
        else if (Token->Kind == TokenKind_Signed)
        {
            if (DetectedSigned)
            {
                Println(Str("ERROR: multiple 'signed' specifiers"));
                Exit(1);
            }

            if (DetectedUnsigned)
            {
                Println(Str("ERROR: 'signed' and 'unsigned' specifiers used together"));
                Exit(1);
            }

            TypeSpec.Signed = true;
            DetectedUnsigned = true;

            *ParseAt = Token->Next;
        }
        else if (Token->Kind == TokenKind_Long)
        {
            if (DetectedShort || DetectedChar)
            {
                Println(Str("ERROR: multiple types specified in declaration"));
                Exit(1);
            }

            if (DetectedLongLong)
            {
                Println(Str("ERROR: too many 'long' in declaration"));
                Exit(1);
            }

            if (DetectedLong)
            {
                DetectedLongLong = true;
                TypeSpec.Bytes = 8;
            }
            else
            {
                DetectedLong = true;
                TypeSpec.Bytes = 4;
            }

            *ParseAt = Token->Next;
        }
        else if (Token->Kind == TokenKind_Int)
        {
            if (DetectedInt)
            {
                Println(Str("ERROR: multiple 'int' in declaration"));
                Exit(1);
            }

            if (!DetectedChar && !DetectedShort && !DetectedLong)
            {
                TypeSpec.Bytes = 4;
            }

            DetectedInt = true;
            *ParseAt = Token->Next;
        }
        else if (Token->Kind == TokenKind_Short)
        {
            if (DetectedShort)
            {
                Println(Str("ERROR: multiple 'short' in declaration"));
                Exit(1);
            }

            if (DetectedChar || DetectedLong)
            {
                Println(Str("ERROR: multiple types specified in declaration"));
                Exit(1);
            }

            TypeSpec.Bytes = 2;
            DetectedShort = true;

            *ParseAt = Token->Next;
        }
        else if (Token->Kind == TokenKind_Char)
        {
            if (DetectedChar)
            {
                Println(Str("ERROR: multiple 'char' in declaration"));
                Exit(1);
            }

            if (DetectedShort || DetectedLong)
            {
                Println(Str("ERROR: multiple types specified in declaration"));
                Exit(1);
            }

            TypeSpec.Bytes = 1;
            DetectedChar = true;

            *ParseAt = Token->Next;
        }
        else
        {
            break;
        }
    }

    return (TypeSpec);
}

local node* ParseStatement(token** ParseAt);

local node* ParseBlock(token** ParseAt)
{
    token* Token = *ParseAt;

    if (Token->Kind != '{')
    {
        Println(Str("ERROR: expected '{' at start of block"));
        Exit(1);
    }

    node* BlockNode = AllocateNode(NodeKind_Block, Token);

    *ParseAt = Token->Next; // NOTE(vak): Skip '{'

    node* FirstStatement = 0;
    node* LastStatement = 0;

    for (;;)
    {
        Token = *ParseAt;
        if ((Token->Kind == TokenKind_EOF) || (Token->Kind == '}'))
            break;

        node* Statement = ParseStatement(ParseAt);
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

    if (Token->Kind != '}')
    {
        Println(Str("ERROR: missing '}' at end of block"));
        Exit(1);
    }

    *ParseAt = Token->Next;

    BlockNode->FirstStatement = FirstStatement;

    return (BlockNode);
}

local node* ParseStatement(token** ParseAt)
{
    token* Token = *ParseAt;

    node* Node = 0;

    type_spec TypeSpec = ParseDeclarationSpecifiers(ParseAt);
    if (TypeSpec.Bytes)
    {
        Node = ParseDeclaration(ParseAt, TypeSpec);

        Token = *ParseAt;
        if (Token->Kind != ';')
        {
            Println(Str("ERROR: expected ';' at end of statement"));
            Exit(1);
        }

        *ParseAt = Token->Next;
    }
    else if (Token->Kind == TokenKind_If)
    {
        Node = AllocateNode(NodeKind_If, Token);

        *ParseAt = Token->Next;
        Token = *ParseAt;

        if (Token->Kind != '(')
        {
            Println(Str("ERROR: expected if conditional to start with '('"));
            Exit(1);
        }

        Node->IfCond = ParseExpression(ParseAt);
        Node->IfThen = ParseStatement(ParseAt);

        Token = *ParseAt;

        if (Token->Kind == TokenKind_Else)
        {
            *ParseAt = Token->Next;
            Node->IfElse = ParseStatement(ParseAt);
        }
    }
    else if (Token->Kind == TokenKind_For)
    {
        Node = AllocateNode(NodeKind_For, Token);

        *ParseAt = Token->Next;
        Token = *ParseAt;

        if (Token->Kind != '(')
        {
            Println(Str("ERROR: expected '(' after for"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        // NOTE(vak): For loop init clause can either be a declaration or
        // an expression

        type_spec TypeSpec = ParseDeclarationSpecifiers(ParseAt);
        if (TypeSpec.Bytes)
        {
            Node->ForInit = ParseDeclaration(ParseAt, TypeSpec);

        }
        else
        {
            Node->ForInit = ParseExpression(ParseAt);
        }

        Token = *ParseAt;
        if (Token->Kind != ';')
        {
            Println(Str("ERROR: expected ';' at end of statement"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        // NOTE(vak): For loop condition

        Node->ForCond = ParseExpression(ParseAt);

        Token = *ParseAt;
        if (Token->Kind != ';')
        {
            Println(Str("ERROR: expected ';' at end of statement"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        // NOTE(vak): For loop iteration update

        Node->ForIter = ParseExpression(ParseAt);

        Token = *ParseAt;
        if (Token->Kind != ')')
        {
            Println(Str("ERROR: missing ')' in for loop"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        Node->ForBody = ParseStatement(ParseAt);
    }
    else if (Token->Kind == TokenKind_While)
    {
        Node = AllocateNode(NodeKind_For, Token);

        *ParseAt = Token->Next;
        Token = *ParseAt;

        if (Token->Kind != '(')
        {
            Println(Str("ERROR: expected '(' after while"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        Node->ForCond = ParseExpression(ParseAt);

        Token = *ParseAt;

        if (Token->Kind != ')')
        {
            Println(Str("ERROR: expected ')' after while conditional"));
            Exit(1);
        }

        *ParseAt = Token->Next;

        Node->ForBody = ParseStatement(ParseAt);
    }
    else if (Token->Kind == '{')
    {
        Node = ParseBlock(ParseAt);
    }
    else
    {
        Node = ParseExpression(ParseAt);

        Token = *ParseAt;
        if (Token->Kind != ';')
        {
            Println(Str("ERROR: expected ';' at end of statement"));
            Exit(1);
        }

        *ParseAt = Token->Next;
    }

    return (Node);
}

local node* Parse(token* FirstToken)
{
    token* ParseAt = FirstToken;

    node* First = 0;
    node* Last = 0;

    while (ParseAt->Kind != TokenKind_EOF)
    {
        node* Statement = ParseStatement(&ParseAt);
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

