# Compiler Dungeon
This project is aimed at trying out and implementing various compiler techniques.

The source language I wish to be compile should look like C99 with core functionality 
such as arithmetic operations, types, arrays, structures, enums, conditions, switches, loops etc. 
( I have no plans to implement dynamic allocation as of now. )

My goals for this project are:
- Play with union-style ast structures opposed to the classical inheritance style ast nodes
- Learn SSA construction algorithms and construct a CFG
- Transform the SSA CFG into SSA Sea Of Nodes IR
- Perform peephole optimizations on the SoN representation
- Implement dataflow analysis framework and perform simple analyses such as cse, dce, liveness, ...
- Compilation to RISC-V assembly
