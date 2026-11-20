// expect: result: 340
// 20 variables and 4 partial sums alive at once: more values than a register file would hold.
fn main() -> i32 {
    let a1 = 1;  let a2 = 2;  let a3 = 3;  let a4 = 4;  let a5 = 5;
    let a6 = 6;  let a7 = 7;  let a8 = 8;  let a9 = 9;  let a10 = 10;
    let a11 = 11; let a12 = 12; let a13 = 13; let a14 = 14; let a15 = 15;
    let a16 = 16; let a17 = 17; let a18 = 18; let a19 = 19; let a20 = 20;
    let s1 = a1 + a2 + a3 + a4 + a5;
    let s2 = a6 + a7 + a8 + a9 + a10;
    let s3 = a11 + a12 + a13 + a14 + a15;
    let s4 = a16 + a17 + a18 + a19 + a20;
    s1 + s2 + s3 + s4 + a1 * a20 + a10 * a11
}
