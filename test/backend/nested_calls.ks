// expect: result: 3551
// `base` is needed again after every call: nothing may keep it in a register a call clobbers.
fn twice(x: i32) -> i32 { x * 2 }

fn add3(a: i32, b: i32, c: i32) -> i32 { a + b + c }

fn main() -> i32 {
    let base = 5;
    let r = add3(twice(base), twice(twice(base)), base);
    let mut i = 0;
    let mut acc = 0;
    while i < 3 {
        acc = acc + twice(i + base) + base;
        i = i + 1;
    }
    r * 100 + acc
}
