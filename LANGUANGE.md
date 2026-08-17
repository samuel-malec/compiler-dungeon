# Kerosene Language Specification

## Overview

Kerosene is a statically typed, expression-oriented programming language with support for:

* Functions and generic functions
* Structs and enums
* Mutable and immutable variables
* References
* Arrays
* Pattern matching
* Control-flow expressions
* Loops
* Expression-oriented blocks

---

## Grammar

The grammar below is written in EBNF.

### Program Structure

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
    ::= "->" type
    ;
```

### Structs

```ebnf
structDeclaration
    ::= "struct" IDENTIFIER genericParams? "{"
            (field ("," field)* ","?)?
        "}"
    ;

field
    ::= IDENTIFIER ":" type
    ;
```

Example:

```text
struct Point {
    x: i32,
    y: i32,
}
```

### Enums

```ebnf
enumDeclaration
    ::= "enum" IDENTIFIER genericParams? "{"
            enumMember ("," enumMember)* ","?
        "}"
    ;

enumMember
    ::= IDENTIFIER ("(" type ("," type)* ")")?
    ;
```

Example:

```text
enum Option<T> {
    Some(T),
    None,
}
```

### Global Variables

```ebnf
globalVariableDeclaration
    ::= "static" "mut"? IDENTIFIER ":" type "=" expression ";"
    ;
```

Examples:

```text
static answer: i32 = 42;

static mut counter: i32 = 0;
```

---

# Types

```ebnf
type
    ::= primitiveType
     | namedType
     | arrayType
     | referenceType
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
    ::= IDENTIFIER genericArgs? "?"
    ;

genericArgs
    ::= "<" type ("," type)* ">"
    ;

arrayType
    ::= type "[" "]"
    ;

referenceType
    ::= "&" "mut"? type
    ;
```

Examples:

```text
i32
bool
Point
Option<i32>
Point?
i32[]
&i32
&mut i32
```

---

# Statements

Statements are used to perform actions without necessarily producing a value.

```ebnf
statement
    ::= letStatement
     | returnStatement
     | breakStatement
     | continueStatement
     | expressionStatement
     ;

letStatement
    ::= "let" "mut"? IDENTIFIER (":" type)? "=" expression ";"
    ;

returnStatement
    ::= "return" expression? ";"
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

### Variable Declarations

Variables are immutable by default.

```text
let x = 10;
```

Mutable variables can be declared using `mut`:

```text
let mut x = 10;

x = 20;
```

An explicit type may optionally be provided:

```text
let x: i32 = 10;
let mut counter: i32 = 0;
```

### Return

A function can explicitly return a value:

```text
return x;
```

A function can also return without a value:

```text
return;
```

### Break and Continue

Loops can be terminated with `break`:

```text
loop {
    if condition {
        break;
    }
}
```

or skip to the next iteration with `continue`:

```text
while condition {
    if skip {
        continue;
    }

    do_something();
}
```

### Expression Statements

Expressions can be used as statements by terminating them with `;`:

```text
x = x + 1;
foo();
array[i] = 42;
```

---

# Blocks

A block consists of zero or more statements followed by an optional final expression.

```ebnf
block
    ::= "{"
            statement*
            expression?
        "}"
    ;
```

The final expression of a block is its value.

For example:

```text
{
    let x = 10;
    let y = 20;

    x + y
}
```

has the value `30`.

An explicit semicolon turns an expression into a statement:

```text
{
    let x = 10;
    x + 1;
}
```

The block above does not have a final expression.

This makes Kerosene expression-oriented while still supporting conventional statements.

---

# Expressions

```ebnf
expression
    ::= assignmentExpression
    ;

assignmentExpression
    ::= logicalOrExpression (assignOp assignmentExpression)?
    ;

assignOp
    ::= "="
     | "+="
     | "-="
     | "*="
     | "/="
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
```

---

# Struct Literals

```ebnf
structLiteral
    ::= IDENTIFIER "{"
            (structLiteralField ("," structLiteralField)* ","?)?
        "}"
    ;

structLiteralField
    ::= IDENTIFIER ":" expression
    ;
```

Example:

```text
let p = Point {
    x: 10,
    y: 20,
};
```

---

# Conditional Expressions

`if` is an expression and can therefore produce a value.

```ebnf
ifExpression
    ::= "if" expression block
        ("else" (ifExpression | block))?
    ;
```

Example:

```text
let max = if a > b {
    a
} else {
    b
};
```

An `if` without an `else` can be used as a statement:

```text
if x > 10 {
    println(x);
}
```

Nested conditions are supported:

```text
if x > 0 {
    1
} else if x < 0 {
    -1
} else {
    0
}
```

---

# Match Expressions

```ebnf
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
```

Example:

```text
match value {
    Some(x) => x,
    None => 0,
}
```

Patterns can also match literals:

```text
match x {
    0 => 10,
    1 => 20,
    _ => 30,
}
```

---

# Loops

## Infinite Loop

```ebnf
loopExpression
    ::= "loop" block
    ;
```

Example:

```text
loop {
    if done {
        break;
    }

    work();
}
```

## While Loop

```ebnf
whileExpression
    ::= "while" expression block
    ;
```

Example:

```text
while x < 10 {
    x += 1;
}
```

Both `loop` and `while` are expressions and may eventually support values produced through `break`.

---

# Literals

```ebnf
literal
    ::= INTEGER
     | FLOAT
     | STRING
     | "true"
     | "false"
    ;
```

Examples:

```text
42
3.14
"hello"
true
false
```

---

# Example Programs

## Maximum of Two Values

```text
fn max(a: i32, b: i32) -> i32 {
    if a > b {
        a
    } else {
        b
    }
}
```

## Fibonacci

```text
fn fib(n: i32) -> i32 {
    if n < 2 {
        n
    } else {
        fib(n - 1) + fib(n - 2)
    }
}
```

## Mutable Variables

```text
fn sum_to(n: i32) -> i32 {
    let mut sum = 0;
    let mut i = 0;

    while i <= n {
        sum += i;
        i += 1;
    }

    sum
}
```

## Structs

```text
struct Point {
    x: i32,
    y: i32,
}

fn distance_squared(p: Point) -> i32 {
    p.x * p.x + p.y * p.y
}
```

## Enums and Matching

```text
enum Option<T> {
    Some(T),
    None,
}

fn unwrap_or(value: Option<i32>, default: i32) -> i32 {
    match value {
        Some(x) => x,
        None => default,
    }
}
```

---

# Design Notes

Kerosene is intentionally **expression-oriented**. Most constructs that control control-flow can also produce values.

For example:

```text
let x = if condition {
    foo()
} else {
    bar()
};
```

Blocks follow the same principle:

```text
let x = {
    let a = 10;
    let b = 20;

    a + b
};
```

Statements are primarily used for operations whose result is not needed:

```text
let mut x = 0;

while x < 10 {
    x += 1;
}
```

The language currently does **not** specify an effect system. Function return types therefore contain only the value type:

```text
fn foo() -> i32 {
    42
}
```

rather than carrying an effect row.

---

# Complete Grammar

For convenience, the complete grammar is collected below.

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
    ::= "->" type
    ;

structDeclaration
    ::= "struct" IDENTIFIER genericParams? "{"
            (field ("," field)* ","?)?
        "}"
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
    ;

type
    ::= primitiveType
     | namedType
     | arrayType
     | referenceType
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
    ::= IDENTIFIER genericArgs? "?"
    ;

genericArgs
    ::= "<" type ("," type)* ">"
    ;

arrayType
    ::= type "[" "]"
    ;

referenceType
    ::= "&" "mut"? type
    ;

statement
    ::= letStatement
     | returnStatement
     | breakStatement
     | continueStatement
     | expressionStatement
    ;

letStatement
    ::= "let" "mut"? IDENTIFIER (":" type)? "=" expression ";"
    ;

returnStatement
    ::= "return" expression? ";"
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

block
    ::= "{"
            statement*
            expression?
        "}"
    ;

expression
    ::= assignmentExpression
    ;

assignmentExpression
    ::= logicalOrExpression (assignOp assignmentExpression)?
    ;

assignOp
    ::= "="
     | "+="
     | "-="
     | "*="
     | "/="
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
    ::= IDENTIFIER "{"
            (structLiteralField ("," structLiteralField)* ","?)?
        "}"
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
    ::= INTEGER
     | FLOAT
     | STRING
     | "true"
     | "false"
    ;
```
