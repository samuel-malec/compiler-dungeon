// expect: result: 204
fn weigh(a: i32, b: i32, c: i32, d: i32, e: i32, f: i32, g: i32, h: i32) -> i32 {
    a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8
}

fn main() -> i32 {
    weigh(1, 2, 3, 4, 5, 6, 7, 8)
}
