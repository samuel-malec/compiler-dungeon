// expect: result: 125250
// 500 nested calls: each frame must keep its own return address.
fn sum(n: i32) -> i32 {
    if n == 0 { 0 } else { n + sum(n - 1) }
}

fn main() -> i32 {
    sum(500)
}
