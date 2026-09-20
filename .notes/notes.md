## Frontend
    - There is a Pratt parser for expressions ready, just extend it with all the operators ... 
    - Create better diagnostics and error reporting

## Semantics

    - Typechecking:
        Enforce linear types (we don't have any non-primitive type yet, so this is currently out of our reach )
    - We could allow unitialized variables, but report errors once unitialized variables are used
    - We could add `const` - a compile-time constant, that gets replaced for every its occurence in the code

# Analyses and Optimizations
    - Implement Control flow simplification 
    - Implement general scaffolding around analyses, such as lattice and forward/backward analysis driver...

## Docs
    Topics to write up about:
        - Pratt Parsing
        - Bidirectional Type Checking
        - SCCP analysis
