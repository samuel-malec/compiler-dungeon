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
    ::= "fn" IDENTIFIER genericParams? "(" parameterList? ")" returnType? block
    ;
genericParams
    ::= "<" IDENTIFIER ("," IDENTIFIER)* ">"
    ;
parameterList
    ::= parameter ("," parameter)*
    ;
parameter
    ::= IDENTIFIER ":" type
    ;
returnType
    ::= "->" type effectRow?
    ;
effectRow
    ::= "/" IDENTIFIER ("+" IDENTIFIER)*
    ;

structDeclaration
    ::= "struct" IDENTIFIER genericParams? "{" (field ("," field)* ","?)? "}"
    ;
field
    ::= IDENTIFIER ":" type
    ;

enumDeclaration
    ::= "enum" IDENTIFIER genericParams? "{"
            enumMember ("," enumMember)* ","?
        "}"
    ;
enumMember
    ::= IDENTIFIER ("(" type ("," type)* ")")?
    ;

globalVariableDeclaration
    ::= "static" "mut"? IDENTIFIER ":" type "=" expression ";"
    
 ```

 ### Types
```ebnf
type
    ::= primitiveType
     | namedType
     | arrayType
     | ::= IDENTIFIER genericArgs?
     | "&" "mut"? type
    ;

genericArgs
    ::= "<" type ("," type)* ">"
    ;;

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
    ::= logicalOrExpression (assignOp assignmentExpression)?
    ;
assignOp
    ::= "=" | "+=" | "-=" | "*=" | "/="
    ;
logicalOrExpression
    ::= logicalAndExpression ("||" logicalAndExpression)*
    ;
logicalAndExpression
    ::= equalityExpression ("&&" equalityExpression)*
    ;
equalityExpression
    ::= relationalExpression (("==" | "!=") relationalExpression)*
    ;
relationalExpression
    ::= additiveExpression (("<" | "<=" | ">" | ">=") additiveExpression)*
    ;
additiveExpression
    ::= multiplicativeExpression (("+" | "-") multiplicativeExpression)*
    ;
multiplicativeExpression
    ::= unaryExpression (("*" | "/" | "%") unaryExpression)*
    ;
unaryExpression
    ::= ("-" | "!" | "&" "mut"?) unaryExpression
     | postfixExpression
    ;
postfixExpression
    ::= primaryExpression postfixOp*
    ;
postfixOp
    ::= "." IDENTIFIER
     | "(" argumentList? ")"
     | "[" expression "]"
    ;
argumentList
    ::= expression ("," expression)*
    ;
primaryExpression
    ::= literal
     | IDENTIFIER
     | structLiteral
     | "(" expression ")"
     | ifExpression
     | matchExpression
     | loopExpression
     | whileExpression
     | block
    ;

structLiteral
    ::= IDENTIFIER "{" (structLiteralField ("," structLiteralField)* ","?)? "}"
    ;
structLiteralField
    ::= IDENTIFIER ":" expression
    ;

ifExpression
    ::= "if" expression block
        ("else" (ifExpression | block))?
    ;

matchExpression
    ::= "match" expression "{"
            matchArm ("," matchArm)* ","?
        "}"
    ;
matchArm
    ::= pattern "=>" expression
    ;
pattern
    ::= IDENTIFIER ("(" pattern ("," pattern)* ")")?
     | literal
     | "_"
    ;

loopExpression
    ::= "loop" block
    ;
whileExpression
    ::= "while" expression block
    ;

literal
    ::= INTEGER | FLOAT | STRING | "true" | "false"
    ;
```

## Example program
```
fn max(a: i32, b: i32) -> i32 {
    if a > b { a } else { b }
}

fn fib(n: i32) -> i32 {
    if n < 2 {
        n
    } else {
        fib(n - 1) + fib(n - 2)
    }
}
```
