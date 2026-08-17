enum Option<T> {
    Some(T),
    None,
}

fn unwrap_or(value: Option<i32>, default: i32) -> i32 {
    match value {
        Some(x) => x,
        None => default,
    }
}

fn main() -> i32 {
    let v: Option<i32> = Some(5);
    return unwrap_or(v, 0);
}

