def main() -> i32 {
    let mut x = 0;

    while (true) {
        if (x == 10) {
            break;
        }

        x = x + 1;
    }

    return x;
}