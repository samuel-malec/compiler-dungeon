fn id(a: i8) -> i8 { a }

fn arithmetic() -> i8 {
    let x: i8 = 10;
    let y = x + 1;
    let z = 2 * y;
    let w = x - -1;
    z
}

fn compare(x: u8) -> bool {
    let a = x < 100;
    let b = 100 > x;
    x == 0
}

fn branch(x: u8) -> i8 {
    if x == 0 { 1 } else { 2 }
}

fn nested(x: i64) -> i64 {
    let y = x + (1 + 2) * 3;
    y
}

fn call_result(x: i8) -> i8 {
    let y = id(x) + 1;
    y
}

fn unannotated() -> i32 {
    let y = 1 + 2;
    let z = -y;
    z
}
