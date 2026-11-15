// expect: result: true
fn main() -> bool {
    let zero = 0;
    true || (1 / zero == 1)
}
