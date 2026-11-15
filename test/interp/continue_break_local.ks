// expect: result: 131
fn main() -> i32 {
    let mut n = 0;
    let mut s = 0;
    while n < 10 {
        n = n + 1;
        if n == 3 { continue; }
        let k = n * n;
        if k > 50 { break; }
        s = s + k;
    }
    s
}
