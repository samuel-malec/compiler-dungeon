// expect: result: 30
// u64 values above 2^63 are negative as signed 64-bit numbers: only the unsigned instructions
// (sltu, divu, remu, srl) get these right.
fn lt(a: u64, b: u64) -> bool { a < b }

fn ge(a: u64, b: u64) -> bool { a >= b }

fn half(a: u64) -> u64 { a / 2 }

fn shift(a: u64) -> u64 { a >> 1 }

fn rem(a: u64, b: u64) -> u64 { a % b }

fn main() -> i32 {
    let m: u64 = 0 - 1;
    let mut r = 0;
    if lt(m, 1) { r = r + 1; }
    if ge(m, 1) { r = r + 2; }
    if half(m) == 9223372036854775807 { r = r + 4; }
    if shift(m) == 9223372036854775807 { r = r + 8; }
    if rem(m, 10) == 5 { r = r + 16; }
    r
}
