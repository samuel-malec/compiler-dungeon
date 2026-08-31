def main() -> i32 {
    let mut x: i32 = 42;
    let mut y: i32 = x + 1;
    if x > 0 {
        y = y + 1;
    }
    // punctuators and operators
    x = x + y * (x - 2) / 3 % 5;
}
