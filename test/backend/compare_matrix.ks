// expect: result: 3875
// Every comparison, signed and unsigned, including the equal case where <= and >= must hold.
fn main() -> i32 {
    let a: i32 = -5;
    let b: i32 = 3;
    let mut r = 0;
    if a < b { r = r + 1; }
    if a <= b { r = r + 2; }
    if a > b { r = r + 4; }
    if a >= b { r = r + 8; }
    if a == b { r = r + 16; }
    if a != b { r = r + 32; }
    let x: u32 = 4000000000;
    let y: u32 = 3;
    if x < y { r = r + 64; }
    if x <= y { r = r + 128; }
    if x > y { r = r + 256; }
    if x >= y { r = r + 512; }
    if b <= b { r = r + 1024; }
    if b >= b { r = r + 2048; }
    r
}
