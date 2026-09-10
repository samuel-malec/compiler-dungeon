def foo() -> i32 {
  let x = 10;
  let y = 20;
  max(x, y)
}

def max(a: i32, b: i32) -> i32 {
  if a < b {
      b
  } else {
      a
  }
}
