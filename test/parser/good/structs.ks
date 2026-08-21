struct Point {
    x: i32,
    y: i32,
}

def distance_squared(p: Point) -> i32 {
    p.x * p.x + p.y * p.y
}

def main() -> i32 {
    let p: Point = Point { x: 3, y: 4 };
    return distance_squared(p);
}
