// expect: trap: signed division overflow
fn main() -> i32 { let m = 0 - 2147483647 - 1; let d = -1; m / d }
