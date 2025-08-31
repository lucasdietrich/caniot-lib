use core::fmt::Debug;
use core::num::NonZeroU8;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Handle(NonZeroU8);

impl Handle {
    pub fn new(handle: u8) -> Option<Self> {
        NonZeroU8::new(handle).map(Self)
    }

    pub(crate) unsafe fn new_unchecked(handle: u8) -> Self {
        Self(unsafe { NonZeroU8::new_unchecked(handle) })
    }

    pub(crate) fn get(&self) -> u8 {
        self.0.get()
    }
}
