struct Pair<T> {
    first: T,
    second: T,
}

enum Option<T> {
    Some(T),
    None,
}

static mut counter: i32 = 0;

def choose<T>(value: Option<T>, fallback: i32[]) -> i32 {
    let greeting = "hello";
    let ratio = 3.14;
    let point = Pair { first: value, second: value };

    match value {
        Some(item) => 1,
        None => 0,
        _ => 2,
    }
}
