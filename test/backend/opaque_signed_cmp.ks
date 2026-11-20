// expect: result: 173
// <= and >= must hold for equal operands; > must not.
fn lt(a: i32, b: i32) -> bool { a < b }

fn le(a: i32, b: i32) -> bool { a <= b }

fn ge(a: i32, b: i32) -> bool { a >= b }

fn gt(a: i32, b: i32) -> bool { a > b }

fn main() -> i32 {
    let mut r = 0;
    if le(3, 3) { r = r + 1; }
    if le(4, 3) { r = r + 2; }
    if le(2, 3) { r = r + 4; }
    if ge(3, 3) { r = r + 8; }
    if ge(2, 3) { r = r + 16; }
    if ge(4, 3) { r = r + 32; }
    if gt(3, 3) { r = r + 64; }
    if gt(4, 3) { r = r + 128; }
    if gt(0 - 4, 3) { r = r + 256; }
    if lt(0 - 4, 3) { r = r + 512; }
    r - 512
}
