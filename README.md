# Compiler Dungeon
This project is aimed at learning and utilizing various compiler techniques to create an optimizing compiler for further research and experimental purposes.
The source language is not really important for this project, but it is has rust-like [syntax](LANGUAGE.md) with no additional libraries ( as of now )

My current goals for this project are:
- Write a recursive descent parser ( potentially, explore Pratt-parsing  ) ✔️
- Write a type-checker ( potentially with type inference ) ✔️ 
- Learn algorithms for to-SSA and from-SSA conversion ✔️
- Learn Sea of Nodes concepts and construct a SoN representation from AST
- Perform optimizations on the SSA CFG as well as SoN and compare the difficulty of implementing optimizations in both of these representations
- Implement dataflow analysis framework and perform simple analyses such as cse, dce, liveness, ...
- Lower to RICV5 ? MLIR ? X86 ?

My future goals for this project:
- Play with the type system - implementing a linear (affine) rust-like type-system could be fun, also implementing generics is something I wanna try out
- Additional language constructs
- Implement additional analyses and optimizations... ( absint/symexec framework ? )
- Write a good suite of benchmark programs and examine how individual analyses and their order impact the executable size and performance
- I would love to implement an actual runtime with the motivation to create a JIT compiler later,
   but since I excluded dynamic allocation from my list of thing to implement, and this is an AOT compiler,
   it would require me to do a lot more work, which seems demanding for a side project.
