def main() -> i32 {
    let mut total: i32 = 1;

    {
        let increment: i32 = 2;
        total += increment;
    };

    total
}
