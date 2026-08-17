fn main() -> i32
{
    let mut a = 5;
    let mut b = 4;
    let mut c = 6;
    let mut d = 10;
    a = b + c;
    b = a - d;
    c = b + c;
    d = a - d;
    return d;
}
