## Frontend
    - I think the syntax design is pretty solid (we don't want to reinvent the wheel)
    - Optional task is to whip out a Pratt parser for expression parsing

## Semantics
    - Enforce linear types (we don't have any non-primitive type yet, so this is currently out of our reach )
    - Allow unitialized variables, but report errors once unitialized variables are used
    - We could add `const` - a compile-time constant, that gets replaced for every its occurence in the code

# Analyses and Optimizations
    - Simple analyses next such as: Constant Folding / Constant Propagation

## Blog & Readme
    - Add compilation pipeline to readme
    - Create a blog series about implementation notes and visualisation of the IR
