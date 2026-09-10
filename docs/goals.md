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
## Diagnostics
```
[wip] Add a mechanism that allows creating nicer diagnostics like warnings/errors 
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
[wip] Constant folding
[] SCCP
[X] DCE
[] CSE
[] GVN
[] Copy propagation
[wip] CFG simplification
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
