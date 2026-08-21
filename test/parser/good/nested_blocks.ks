def main(a: i32, b: i32) -> i32
{
    let x : i32 = 10;
    {
        let x : i32 = 20;
        let a : i32 = x + 10;
    };
    return x;
}
