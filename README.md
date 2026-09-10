# Compiler Dungeon
This project is aimed at learning and utilizing various compiler techniques to create an optimizing compiler for further research and experimental purposes.
My current personal goals for this project are outlined in the [goals](docs/goals.md) document.

## Kerosene Language
Kerosene is a custom programming language which we are compiling in this project.
The syntax of Keresone is inspired off of Rust and Python, because I believe that it is modern and has exactly what I look for in a language
(at the end of the day I don't plan invent an entirely new syntax in this project)
The entire EBNF format of Kerosene's grammar can be viewed [here](docs/language.md)

## Compiler Overview
As any compiler, this project is divided into individual passes with clearly outlined functionality.
I am hoping to write more detailed explanations of the different representations, although the source code should be pretty
self-explanatory
```text
  Kerosene
     │
     ▼
    AST
     │
     ▼
    HIR
     │
     ▼
     IR
     │
     ▼
   SSA-CFG
     │
     ▼
 Optimizations 
     │
     ▼
# TODO lower to actual target 
```

## Repository Structure
```
src/
├── analysis/    # Analysis passes which run on the IR
├── codegen/     # TODO, this is where lowering to target languages happens (riscv5, arm, x64, wasm...)
├── common/      # Shared utilities and infrastructure
├── driver/      # Compilation pipeline
├── frontend/    # Lexer, Parser, Ast structures
├── hir/         # High IR - a typped, cannonicalized-like ast
├── interpreter/ # TODO: IR interpreter
├── ir/          # Intermediate Representation structures 
├── sema/        # Semantical analysis -> scope analysis, bidirectional typechecking
├── wasm/        # WASM bindings for web visualizer
```

## Interactive Playground
I am hoping to share my notes and technical concepts related to compiler dungeon in my blog:
https://samuel-malec.github.io/blog/2026/08/29/compiler-dungeon/intro. Currently, I have only got the interactive compiler
somehow working, but I hope to polish it and add first posts regarding this project.