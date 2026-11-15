// expect: result: false
fn main() -> bool {
    let zero = 0;
    false && (1 / zero == 1)
}
