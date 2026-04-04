use std::sync::Mutex;

pub struct BlockHeader {
    pub size: usize,
    pub next: Option<Box<BlockHeader>>,
    pub free: u32, // 0 for free and 1 for allocated
    pub debug: i32,
}

static GLOBAL_BASE: Mutex<Option<*mut BlockHeader>> = Mutex::new(None);
