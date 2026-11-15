// expect: result: 21
fn main() -> i32 {
    let mut a = 1;
    let mut b = 2;
    let mut t = 0;
    let mut i = 0;
    while i < 5 {
        t = a;
        a = b;
        b = t;
        i = i + 1;
    }
    a * 10 + b
}
