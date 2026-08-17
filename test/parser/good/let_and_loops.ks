fn sum_to(n: i32) -> i32 {
    let mut sum = 0;
    let mut i = 0;

    while i <= n {
        sum += i;
        i += 1;
    }

    sum
}

fn main() -> i32 {
    return sum_to(10);
}

