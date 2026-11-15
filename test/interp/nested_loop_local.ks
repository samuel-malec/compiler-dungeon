// expect: result: 42
fn main() -> i32 {
    let mut i = 0;
    let mut last = 0;
    while i < 5 {
        let mut j = 0;
        while j < 3 {
            let t = i * 10 + j;
            last = t;
            j = j + 1;
        }
        i = i + 1;
    }
    last
}
