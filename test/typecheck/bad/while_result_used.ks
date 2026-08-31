def main() -> i32 {
    let mut i: i32 = 0;
    let x: i32 = while i < 10 { i += 1; i };
    x
}
