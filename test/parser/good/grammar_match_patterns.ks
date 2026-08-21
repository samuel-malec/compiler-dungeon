enum Token {
    Pair(i32, i32),
    Empty,
}

def classify(token: Token) -> i32 {
    match token {
        Pair(0, _) => 0,
        Pair(left, right) => left,
        Empty => 1,
    }
}

def literals(value: i32) -> i32 {
    match value {
        0 => 10,
        3.14 => 20,
        "text" => 30,
        _ => 40,
    }
}
