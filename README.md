# Compiler Dungeon
This project is aimed at learning and utilizing various compiler techniques to create an optimizing compiler for further research and experimental purposes.
The source language is not really important for this project because I do not have ambitions to create a production-level language which will be used by other people.
Nonetheless, the actual is inspired off of Rust and Python, and its EBNF format can be viewed [here](LANGUAGE.md)

My current goals for this project are:

## Frontend
```
[x] Recursive descent parser
[~] Pratt parser for expressions
[x] Bidirectional Type checker (e.g type inference + checking)
[] Pattern matching
[] Exhaustiveness checking
[] Generics
[] Algebraic data types
[] Affine/linear types
```
## Middle-end
```
[x] CFG
[x] SSA
[] Pass Manager
[] Analysis Manager
```

## SSA Optimizations
```
[] Constant folding
[] SCCP
[] DCE
[] CSE
[] GVN
[] Copy propagation
[] CFG simplification
[] Jump threading
[] LICM
[] Strength reduction
[] Induction-variable analysis
```

## Backend
```
[] Machine IR
[] Instruction selection
[] Calling convention
[] Liveness
[] Linear-scan register allocation
[] Graph-coloring register allocation
[] Spill/reload
[] Stack frames
[] RISC-V code generation
[] ELF generation
```
## Runtime
```
[] Tiny runtime
[] strings
[] arrays
[] file I/O
[] allocator
[] eventually GC / ownership runtime if desired
```

## Experimental
```
[] Benchmark suite
[] Pass ordering experiments
[] Optimization statistics
[] Compile-time measurements
[] Code-size measurements
[] Runtime measurements
[x] IR visualization
[] Differential testing
[] Random/fuzz testing
```


## Project Structure

## Compiler Overview
