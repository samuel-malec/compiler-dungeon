## Frontend

    - There is a Pratt parser for expressions ready, just extend it with all the operators ... 
    - Create better diagnostics and error reporting

## Semantics

    - Improve typechecker - currently lot of things are hardcoded and allowed only for integers, find a way to easily declare
        - What operations are allowed on what types, create a class hierarchy of some kind...
        - Enforce linear types (we don't have any non-primitive type yet, so this is currently out of our reach )
    - We could allow unitialized variables, but report errors once unitialized variables are used
    - We could add `const` - a compile-time constant, that gets replaced for every its occurence in the code

# Analyses and Optimizations

    - Implement SCCP (Sparse Conditional Constant Propagation)
    - Implement general scaffolding around analyses, such as lattice and forward/backward analysis driver...
    - Reasearch different kinds of control flow analysis

## Docs

    - Improve the web visualization with SCCP optimization
