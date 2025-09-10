use core::fmt::Debug;
use std::{mem::MaybeUninit, ops::{Deref, DerefMut}};

use crate::{
    did::DeviceId, error::FailCode, types::{Direction, Endpoint, Type}
};

use caniot_sys as ll;

pub trait Frame {
    fn get_did(&self) -> DeviceId;
    fn get_direction(&self) -> Direction;
    fn get_type(&self) -> Type;
    fn get_endpoint(&self) -> Endpoint;
    fn get_payload(&self) -> &[u8];
}

impl Frame for ll::caniot_frame_t {
    fn get_did(&self) -> DeviceId {
        unsafe { DeviceId::new_unchecked(self.id.cls() as u8, self.id.sid() as u8) }
    }

    fn get_direction(&self) -> Direction {
        self.id.query().into()
    }

    fn get_type(&self) -> Type {
        self.id.type_().into()
    }

    fn get_endpoint(&self) -> Endpoint {
        Endpoint::try_from(self.id.endpoint()).unwrap()
    }

    fn get_payload(&self) -> &[u8] {
        unsafe { core::slice::from_raw_parts(self.buf.as_ptr(), self.len as usize) }
    }
}

#[derive(Clone, PartialEq, Eq)]
pub struct FrameRef<'a>(&'a ll::caniot_frame_t);

impl<'a> FrameRef<'a> {
    pub fn new(frame: &'a ll::caniot_frame_t) -> Self {
        FrameRef(frame)
    }

    /// # Safety
    ///
    /// `frame` must be a valid, non-null pointer to a properly initialized
    /// `caniot_frame_t` for the duration of the call. The pointed structure
    /// must not be mutated concurrently while this function executes (aliasing rules),
    /// and the returned `Frame` borrows the payload bytes immutably for `'a`.
    pub unsafe fn from_ll_unchecked(frame: *const ll::caniot_frame_t) -> Self {
        debug_assert!(!frame.is_null());
        let frame = unsafe { &*frame };
        FrameRef(frame)
    }

    pub fn to_owned(&self) -> OwnedFrame {
        OwnedFrame(*self.0)
    }
}

impl Deref for FrameRef<'_> {
    type Target = ll::caniot_frame_t;

    fn deref(&self) -> &Self::Target {
        self.0
    }
}

impl Debug for FrameRef<'_> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(
            f,
            "FrameRef {{ did: {}, dir: {:?}, type: {:?}, endpoint: {:?}, payload: {:02X?} }}",
            self.get_did(),
            self.get_direction(),
            self.get_type(),
            self.get_endpoint(),
            self.get_payload()
        )
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Request {
    Telemetry {
        endpoint: Endpoint,
    },
    Command {
        endpoint: Endpoint,
        payload: Vec<u8>,
    },
    AttributeRead {
        key: u16,
    },
    AttributeWrite {
        key: u16,
        value: u32,
    },
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ErrorSource {
    Telemetry(Endpoint, Option<u32>),
    Attribute(Option<u16>),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Response {
    Telemetry {
        endpoint: Endpoint,
        payload: Vec<u8>
    },
    Attribute {
        key: u16,
        value: u32,
    },
    Error {
        source: ErrorSource,
        error: Option<FailCode>,
    },
}

pub struct OwnedFrame(ll::caniot_frame_t);

impl Default for OwnedFrame {
    fn default() -> Self {
        let mut frame: MaybeUninit<ll::caniot_frame_t> = MaybeUninit::uninit();
        unsafe { ll::caniot_clear_frame(frame.as_mut_ptr() as *mut ll::caniot_frame_t) };
        OwnedFrame(unsafe { frame.assume_init() })
    }
}

impl Deref for OwnedFrame {
    type Target = ll::caniot_frame_t;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for OwnedFrame {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl OwnedFrame {
    pub fn new_request(data: Request) -> OwnedFrame {
        let mut frame: MaybeUninit<ll::caniot_frame_t> = MaybeUninit::uninit();

        unsafe {
            match data {
                Request::Telemetry { endpoint } => {
                    ll::caniot_build_query_telemetry(frame.as_mut_ptr(), endpoint.into());
                }
                Request::Command { endpoint, payload } => {
                    ll::caniot_build_query_command(frame.as_mut_ptr(), endpoint.into(), payload.as_ptr(), payload.len() as u8);
                }
                Request::AttributeRead { key } => {
                    ll::caniot_build_query_read_attribute(frame.as_mut_ptr(), key);
                }
                Request::AttributeWrite { key, value } => {
                    ll::caniot_build_query_write_attribute(frame.as_mut_ptr(), key, value);
                }
            }

            OwnedFrame(frame.assume_init())
        }
    }
}

impl AsRef<ll::caniot_frame_t> for OwnedFrame {
    fn as_ref(&self) -> &ll::caniot_frame_t {
        &self.0
    }
}

impl AsMut<ll::caniot_frame_t> for OwnedFrame {
    fn as_mut(&mut self) -> &mut ll::caniot_frame_t {
        &mut self.0
    }
}