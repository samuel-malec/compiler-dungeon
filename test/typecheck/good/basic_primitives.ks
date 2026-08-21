def main() -> i32 {
    let left: i32 = 4;
    let right = 2;
    let is_larger: bool = left > right;

    if is_larger {
        left + right
    } else {
        0
    }
}
