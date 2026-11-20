// expect: result: 173
fn le(a: u32, b: u32) -> bool { a <= b }

fn ge(a: u32, b: u32) -> bool { a >= b }

fn gt(a: u32, b: u32) -> bool { a > b }

fn main() -> i32 {
    let mut r = 0;
    if le(3, 3) { r = r + 1; }
    if le(4000000000, 3) { r = r + 2; }
    if le(2, 4000000000) { r = r + 4; }
    if ge(3, 3) { r = r + 8; }
    if ge(2, 4000000000) { r = r + 16; }
    if ge(4000000000, 3) { r = r + 32; }
    if gt(3, 3) { r = r + 64; }
    if gt(4000000000, 3) { r = r + 128; }
    r
}
