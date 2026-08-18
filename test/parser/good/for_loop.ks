fn main() -> i32
{
    let sum : i32 = 0;
    for ( let i : i32 = 0; i < 10; i += 1 )
    {
        sum += i;
    }
    return sum;
}