#[unsafe(no_mangle)]
pub extern "C" fn __assert(statement: bool) {
    if !statement {
        panic!("Assertion failed");
    }
}
