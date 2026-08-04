# Kerosene Language Specification

## Grammar ( EBNF )
```ebnf
program
    ::= toplevel* EOF
    ;

toplevel
    ::= functionDeclaration
     | structDeclaration
     | enumDeclaration
     | globalVariableDeclaration
    ;

functionDeclaration
    ::= "fn" IDENTIFIER "(" parameterList? ")" returnType? block
    ;

returnType
    ::= "->" type
    ;

parameterList
    ::= parameter ("," parameter)*
    ;

parameter
    ::= IDENTIFIER ":" type
    ;

structDeclaration
    ::= "struct" IDENTIFIER "{" field* "}"
    ;

field
    ::= IDENTIFIER ":" type ";"
    ;

enumDeclaration
    ::= "enum" IDENTIFIER "{"
            enumMember ("," enumMember)* ","?
        "}"
    ;

enumMember
    ::= IDENTIFIER
    ;

globalVariableDeclaration
    ::= "let" "global" IDENTIFIER (":" type)? "=" expression ";"
    ;

block
    ::= "{"
            statement*
        "}"
    ;

statement
    ::= block
     | variableDeclaration
     | returnStatement
     | ifStatement
     | whileStatement
     | breakStatement
     | continueStatement
     | expressionStatement
    ;

variableDeclaration
    ::= "let" "mut"? IDENTIFIER
        (":" type)?
        ("=" expression)?
        ";"
    ;

returnStatement
    ::= "return" expression? ";"
    ;

ifStatement
    ::= "if" expression statement
        ("else" statement)?
    ;

whileStatement
    ::= "while" expression statement
    ;

breakStatement
    ::= "break" ";"
    ;

continueStatement
    ::= "continue" ";"
    ;

expressionStatement
    ::= expression ";"
    ;
```

### Types
```ebnf
type
    ::= primitiveType
     | namedType
     | arrayType
     ;

primitiveType
    ::= "i8"
     | "i16"
     | "i32"
     | "i64"
     | "u8"
     | "u16"
     | "u32"
     | "u64"
     | "bool"
    ;

namedType
    ::= IDENTIFIER "?"?
    ;

arrayType
    ::= type "[" "]"
    ;

```

### Expression
```ebnf
expression
    ::= assignmentExpression
    ;

assignmentExpression
    ::= logicalOrExpression
        (assignmentOperator assignmentExpression)?
    ;

assignmentOperator
    ::= "="
     | "+="
     | "-="
     | "*="
     | "/="
    ;

logicalOrExpression
    ::= logicalAndExpression
        ("||" logicalAndExpression)*
    ;

logicalAndExpression
    ::= bitwiseOrExpression
        ("&&" bitwiseOrExpression)*
    ;

bitwiseOrExpression
    ::= bitwiseXorExpression
        ("|" bitwiseXorExpression)*
    ;

bitwiseXorExpression
    ::= bitwiseAndExpression
        ("^" bitwiseAndExpression)*
    ;

bitwiseAndExpression
    ::= equalityExpression
        ("&" equalityExpression)*
    ;

equalityExpression
    ::= relationalExpression
        (("==" | "!=") relationalExpression)*
    ;

relationalExpression
    ::= shiftExpression
        (("<" | "<=" | ">" | ">=") shiftExpression)*
    ;

shiftExpression
    ::= additiveExpression
        (("<<" | ">>") additiveExpression)*
    ;

additiveExpression
    ::= multiplicativeExpression
        (("+" | "-") multiplicativeExpression)*
    ;

multiplicativeExpression
    ::= unaryExpression
        (("*" | "/" | "%") unaryExpression)*
    ;

unaryExpression
    ::= ("-" | "!" | "~") unaryExpression
     | postfixExpression
    ;

postfixExpression
    ::= primaryExpression postfixOperator*
    ;

postfixOperator
    ::= "." IDENTIFIER
     | "[" expression "]"
     | "(" argumentList? ")"
    ;

argumentList
    ::= expression ("," expression)*
    ;

primaryExpression
    ::= INTEGER_LITERAL
     | "true"
     | "false"
     | "null"
     | IDENTIFIER
     | "(" expression ")"
    ;
```

## Example program
```
fn fib(n: i32) -> i32 {
    if n <= 1 {
        return n;
    }

    let mut a = 0;
    let mut b = 1;

    while n > 1 {
        let next = a + b;
        a = b;
        b = next;
        n -= 1;
    }

    return b;
}

fn main() {
    let value = fib(10);
    print(value);
}
```
