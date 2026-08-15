
#pragma once

typedef enum
{
    NodeKind_Nil = 0,

    NodeKind_Integer,

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
} node_kind;

typedef struct node node;
struct node
{
    node_kind Kind;
    token* Token;

    usize Integer;

    node* Left;
    node* Right;
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
    else
    {
        Println(Str("ERROR: syntax error"));
        Exit(1);
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
    else
    {
        Node = ParsePrimary(ParseAt);
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

local node* ParseExpression(token** ParseAt)
{
    node* Node = ParseLogicalOr(ParseAt);
    return (Node);
}

local node* Parse(token* FirstToken)
{
    token* ParseAt = FirstToken;
    node* Node = ParseExpression(&ParseAt);
    return (Node);
}

