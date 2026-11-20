// expect: result: -128
// SCCP folds constants away, so the operands arrive through parameters: the backend must really add.
fn add(a: i8, b: i8) -> i8 { a + b }

fn main() -> i8 { add(127, 1) }
