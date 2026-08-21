def expressions(values: i32[], index: i32, flag: bool) -> i32 {
    let mut total = -values[index] + 3 * 4 / 2 % 5;
    let condition = !flag || total <= 10 && total != 0;
    let chosen = if condition {
        total
    } else if flag {
        1
    } else {
        2
    };

    total += chosen;
    total = (total + values[index]) / 2;
    total
}
