def main() -> i32 {
    let mut x = 1;
    let mut y = 2;
    let mut z = x + y;
    if true {
        y = y - 1;
        x = x + y;
    } else {
         y = y + 1;
         x = y;
    }
    y = x * 2;
    z = z + x;
    z
}