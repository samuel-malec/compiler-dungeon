struct TypeCatalog<T, U> {
    primitive: i64,
    named: T,
    generic: Result<T, U>,
    nullable: U?,
    array: i32[],
    reference: &i8,
    mutable_reference: &mut bool,
}

enum Message<T> {
    Empty,
    Value(T),
    Pair(T, i32),
}

static answer: i32 = 42;
static mut enabled: bool = true;

fn identity<T>(value: T) -> T {
    value
}

fn no_result() {
    return;
}
