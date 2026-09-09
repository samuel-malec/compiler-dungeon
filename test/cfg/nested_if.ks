def sign(x: i32) -> i32 {
    let mut r = 0;
    if x > 0 {
        r = 1;
    } else if x < 0 {
        r = -1;
    } else {
        r = 0;
    }
    r
}
