use crate::error::FailCode;

#[derive(Default, Debug, Clone, PartialEq, Eq)]
pub struct Payload {
    data: [u8; 8],
    len: usize,
}

impl Payload {
    pub fn new(data: &[u8]) -> Result<Self, FailCode> {
        if data.len() > 8 {
            return Err(FailCode::EINVAL);
        }
        let mut payload = Payload {
            data: [0; 8],
            len: data.len(),
        };
        payload.data[..data.len()].copy_from_slice(data);
        Ok(payload)
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn as_ptr(&self) -> *const u8 {
        self.data.as_ptr()
    }

    pub fn as_mut_ptr(&mut self) -> *mut u8 {
        self.data.as_mut_ptr()
    }

    /// # Safety
    ///
    /// `len` must be less than or equal to 8.
    pub unsafe fn set_len(&mut self, len: usize) {
        self.len = len;
    }

    pub fn uninit() -> Self {
        Payload::default()
    }
}

impl AsRef<[u8]> for Payload {
    fn as_ref(&self) -> &[u8] {
        &self.data[..self.len]
    }
}
// Macro to generate From<[u8; N]> implementations for N = 0..=8 without repetition.
macro_rules! impl_from_array_payload {
    ($($n:literal),+) => {
        $(impl From<[u8; $n]> for Payload {
            #[inline]
            fn from(val: [u8; $n]) -> Self {
                let mut data = [0u8; 8];
                // copy (no-op for 0)
                data[..$n].copy_from_slice(&val);
                Payload { data, len: $n }
            }
        })+
    }
}

impl_from_array_payload!(0, 1, 2, 3, 4, 5, 6, 7, 8);

#[cfg(test)]
mod tests {
    use super::Payload;

    #[test]
    fn from_arrays_var_len() {
        let p0 = Payload::from([]);
        assert_eq!(p0.len(), 0);

        let p3 = Payload::from([1u8, 2, 3]);
        assert_eq!(p3.as_ref(), &[1, 2, 3]);
        assert_eq!(p3.len(), 3);

        let p8 = Payload::from([10u8, 11, 12, 13, 14, 15, 16, 17]);
        assert_eq!(p8.len(), 8);
        assert_eq!(p8.as_ref(), &[10, 11, 12, 13, 14, 15, 16, 17]);
    }
}
