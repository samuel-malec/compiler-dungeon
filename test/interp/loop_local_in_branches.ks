// expect: result: 330
fn main() -> i32 {
    let mut i = 0;
    let mut acc = 0;
    while i < 6 {
        let x = i * 2;
        if x > 4 {
            let y = x + 100;
            acc = acc + y;
        } else {
            acc = acc + x;
        }
        i = i + 1;
    }
    acc
}
