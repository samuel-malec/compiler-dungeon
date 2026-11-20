// expect: result: 240
fn shl(a: u8, s: u8) -> u8 { a << s }

fn main() -> u8 { shl(255, 4) }
