# Compiler Dungeon
This project is aimed at learning and utilizing various compiler techniques to create an optimizing compiler for further research and experimental purposes.
My current personal goals for this project are outlined in the [goals](docs/goals.md) document.

## Usage
Compiler dungeon has a CLI interface, with all options listable using `./compiler-dungeon -h`.
To compile a kerosene file, simply type `./compiler-dungeon file.ks`, which will produce a `file.out` binary file. 
The `-o` flag can be used to changed the name of the resulting binary.

## Status
- Support for basic expressions and control flow statements
- Semantic analysis and bidirectional type checking
- Conversion to SSA IR
- Analysis passes on the SSA IR such as: SCCP, DCE, Constant folding/propagation SimplifyCFG,...

Ongoing:
  Writing the ricv64 backend


## Repository Structure
```
src/
├── analysis/    # Analysis passes which run on the IR
├── codegen/     # Generating riscv64
├── common/      # Shared utilities
├── driver/      # Compilation pipeline driver
├── frontend/    # Lexer, Parser, AST structures
├── hir/         # High IR - a typped, cannonicalized-like ast
├── ir/          # Intermediate representation structures 
├── sema/        # Semantical analysis
├── wasm/        # WASM bindings for web visualizer
```

## Interactive Playground
I am hoping to share my notes and technical concepts related to compiler dungeon in my blog:
https://samuel-malec.github.io/blog/2026/08/29/compiler-dungeon/intro. Currently, I have only got the interactive compiler
somehow working, but I hope to polish it and add first posts regarding this project.
