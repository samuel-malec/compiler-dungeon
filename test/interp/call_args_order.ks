// expect: result: 63
fn sub(a: i32, b: i32) -> i32 { a - b }

fn main() -> i32 {
    sub(10, 3) * 10 + sub(3, 10)
}
