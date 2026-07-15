# Compiler Dungeon
This project is aimed at learning and utilizing various compiler techniques to create an optimizing compiler for further research and experimental purposes.

The source language I wish to compile should look like C99 with core functionality 
such as arithmetic operations, primitive types ( so far only int and bool ), structures, enums, conditions, switches, loops etc. 
( I have no plans to implement dynamic allocation as of now. )

My current goals for this project are:
- Handwrite a recursive descent parser
- Play with intermediate representations and their conversions : AST -> HIR -> TAC -> CFG -> SSA-CFG and AST -> SON 
- Learn SSA construction algorithms
- Learn Sea of Nodes concepts and construct a SoN representation from AST
- Perform optimizations on the SSA CFG as well as SoN and compare the difficulty of implementing optimizations in both of these representations
- Implement dataflow analysis framework and perform simple analyses such as cse, dce, liveness, ...
- Compilation to RISC-V assembly

My future goals for this project:
- Well, I can basically scale this project indefinitely, but there are things that I see as possible future improvement
- I can add additional language constructs, richer types, dynamic allocation, arrays...
- I can do always do additional analyses and optimizations... ( absint/symexec framework ? )
- Write a good suite of benchmark programs and examine, how individual analyses and their order impact the executable size and performance
- I would love to implement an actual runtime, but since I excluded dynamic allocation from my list of thing to implement, and this is an AOT compiler,
   it would require me to create my own bytecode, vm, interpreter and plug it on top of dungeon,
   which seems kinda demanding for a side project. I will probably implement this in another project which is more suitable...
  
