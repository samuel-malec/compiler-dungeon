// expect: result: 15
fn main() -> i32 {
    let mut i = 0;
    let mut sum = 0;
    while i < 5 {
        let t = i + 1;
        sum = sum + t;
        i = i + 1;
    }
    sum
}
