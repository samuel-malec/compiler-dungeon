fn main(a: i32, b: i32) -> i32
{
    let x = 10;
    {
        let x = 20;
        let a = x + 10;
    }
    return x;
}
