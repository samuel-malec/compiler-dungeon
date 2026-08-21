def main() -> i32 {
    let mut count: i32 = 0;

    loop {
        count += 1;
        if count == 3 {
            break;
        }
        continue;
    }

    count
}
