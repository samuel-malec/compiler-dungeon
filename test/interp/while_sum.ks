// expect: result: 55
fn main() -> i32 {
    let mut i = 1;
    let mut sum = 0;
    while i <= 10 {
        sum = sum + i;
        i = i + 1;
    }
    sum
}
