// expect: trap: stack overflow
fn forever(n: i32) -> i32 { forever(n + 1) }

fn main() -> i32 {
    forever(0)
}
