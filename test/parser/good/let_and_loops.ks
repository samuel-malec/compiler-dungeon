def sum_to(n: i32) -> i32 {
    let mut sum : i32 = 0;
    let mut i : i32 = 0;

    while i <= n {
        sum += i;
        i += 1;
    }

    sum
}

def main() -> i32 {
    return sum_to(10);
}

