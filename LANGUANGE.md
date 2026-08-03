# Kerosene Language Specification


## Grammar ( EBNF )
```ebnf

program : toplevel+ EOF;

toplevel : fnDeclaration | structDeclaration | enumDeclaration | globalVarDeclaration

fnDeclaration : 'fn' IDENTIFIER '(' paramList ')' block

structDeclaration : 'struct' IDENTIFIER '{' fieldList '}'  ;

enumDeclaration : 'enum' IDENTIFIER '{ enumList '}';

globalVarDeclaration : 'let global' IDENTIFIER '=' expression;

fieldList : field* ; 

field : IDENTIFER : type;

enumList : IDENTIFIER*;

statement
    : '{' block '}'
    | returnStatement
    | ifStatement
    | whileStatement
    | breakStatement
    | continueStatement
    | expressionStatement
    | varDeclaration
    ;


returnStatement : 'return' expression ';' ;

ifStatement : 'if' '(' expression ')' statement ('else' statement)? ;

whileStatement : 'while' '(' expression ')' statement ;

breakStatement : 'break' ';' ;

continueStatement : 'continue' ';' ;

expressionStatement
    : type IDENTIFIER ';'
    | type IDENTIFIER '=' expression ';'
    |      IDENTIFIER '=' expression ';'
    |                     expression
    ;

varDeclaration: 'let' IDENTIFIER : type = expression;

PRIMTYPE
    : 'int'
    | 'i8'
    | 'i16'
    | 'i32'
    | 'i64'
    | 'u8'
    | 'u16'
    | 'u32'
    | 'flt'
    | 'f32'
    | 'f64'
    | 'bool'
    ;

type
    : PRIMTYPE
    | typeName '?'?
    | typeName nestedArray+
    ;

nestedArray : '?'?  '[]'* ;

typeName : IDENTIFIER ;


expression : bitWiseExpression ;

bitWiseExpression
    : comparisonExpression ( '&' | '|' | '^' ) comparisonExpression)*
    ;
    
comparisonExpression
    : shiftExpression (('==' | '!='| '>'| '<'| '>='| '<=') shiftExpression)*
    ;

shiftExpression
    : additiveExpression ( '<<' | '>>>' | '>>' ) additiveExpression)*
    ;
    
additiveExpression
    : multiplicativeExpression (('+' | '-') multiplicativeExpression)*
    ;

multiplicativeExpression
    : unaryExpression (('*' | '/') unaryExpression)*
    ;

unaryExpression
    : ('-') unaryExpression
    | '!' unaryExpression
    | primaryExpression postAssign
    ;

primaryExpression
    : INTEGER_LITERAL
    | FLOAT_LITERAL
    | '(' expression ')'
    | 'true'
    | 'false'
    | 'null'
    | newExpression
    | IDENTIFIER
    ;

newExpression 
    : 'new' IDENTIFIER [ '{' block '}' ]
    | 'new' IDENTIFIER '[' expression ']'
    ;

postAssign : postFix [ '=' expression ] | [ '#' ];

postFix
    : '.' IDENTIFIER      postFix
    | '[' expression ']'  postFix
    ;


IDENTIFIER : NON_DIGIT (NON_DIGIT | DIGIT)*  ;

INTEGER_LITERAL
    : [1-9]DIGIT*
    | [0]
    ;

NON_DIGIT: [a-zA-Z_];
DIGIT: [0-9];

```