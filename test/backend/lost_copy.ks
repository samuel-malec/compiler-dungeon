// expect: result: 45
// The value of `x` the loop test sees on the last iteration is still needed after the loop, but the
// back edge also overwrites it with the next value: a copy placed at the end of the loop block runs
// on the exit path too unless the critical edge is split.
fn main() -> i32 {
    let mut x = 0;
    let mut old = 0;
    while true {
        old = x;
        x = x + 1;
        if x < 5 { continue; }
        break;
    }
    old * 10 + x
}
