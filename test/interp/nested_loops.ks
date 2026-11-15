// expect: result: 45
fn main() -> i32 {
    let mut i = 0;
    let mut total = 0;
    while i < 10 {
        let mut j = 0;
        while j < i {
            total = total + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    total
}
