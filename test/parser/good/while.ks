def main() -> i32 {
    let mut i: i32 = 0;

    while i < 100 {
        i += 1;
    }

    let x : unit = while i < 100 {
        i += i;
    };

    return i;
}
