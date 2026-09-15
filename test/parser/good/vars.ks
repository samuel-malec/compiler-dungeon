fn main() -> i32
{
    let mut a: i32 = 5;
    let mut b: i32 = 4;
    let mut c: i32 = 6;
    let mut d: i32 = 10;
    a = b + c;
    b = a - d;
    c = b + c;
    d = a - d;
    return d;
}
