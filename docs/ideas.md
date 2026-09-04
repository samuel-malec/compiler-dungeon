## Frontend:
    - Remove curly brackets from the syntax and make the language identation-based to feel more like a scripting language
    - Make function return type optional, but if no type is provided, assign unit type as the actual return type 

## Semantics
    - Allow unitialized variables, but report errors once unitialized variables are used
    - We could add `const` - a compile-time constant, that gets replaced for every its occurence in the code

## Blog & Readme
    - Add compilation pipeline to readme
    - Create a blog series about implementation notes and a little bit y


## Known issues:
This program should compile, but it doesn't.
```
def main() -> i32 {
    let x = 42;
    return x;
}
```