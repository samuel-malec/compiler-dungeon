// expect: out of fuel
fn main() -> i32 {
    let mut x = 0;
    while true {
        x = x + 1;
    }
    x
}
