// expect: result: 231
// Three phis that feed each other in a cycle: a <- b <- c <- a.
fn main() -> i32 {
    let mut a = 1;
    let mut b = 2;
    let mut c = 3;
    let mut i = 0;
    while i < 4 {
        let t = a;
        a = b;
        b = c;
        c = t;
        i = i + 1;
    }
    a * 100 + b * 10 + c
}
