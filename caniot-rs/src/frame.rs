use core::fmt::Debug;

use crate::{
    did::DeviceId,
    types::{Action, Direction, Endpoint, Kind},
};

use caniot_sys as ll;

pub struct Frame<'a> {
    did: DeviceId,
    direction: Direction,
    kind: Kind,
    action: Action,
    endpoint: Endpoint,
    payload: &'a [u8],
}

impl<'a> Frame<'a> {
    pub fn new(did: DeviceId, payload: &'a [u8]) -> Result<Frame<'a>, &'static str> {
        Ok(Frame {
            did,
            direction: Direction::Query,
            kind: Kind::Telemetry,
            action: Action::Write,
            endpoint: Endpoint::ApplicationDefault,
            payload,
        })
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

        let (action, kind) = match frame.id.type_() {
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_COMMAND => (Action::Write, Kind::Telemetry),
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_TELEMETRY => (Action::Read, Kind::Telemetry),
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_READ_ATTRIBUTE => {
                (Action::Read, Kind::Attribute)
            }
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE => {
                (Action::Write, Kind::Attribute)
            }
            _ => panic!("Unknown frame type"),
        };

        let direction = match frame.id.query() {
            ll::caniot_frame_dir_t::CANIOT_QUERY => Direction::Query,
            ll::caniot_frame_dir_t::CANIOT_RESPONSE => Direction::Response,
            _ => panic!("Unknown frame direction"),
        };

        let did = unsafe { DeviceId::new_unchecked(frame.id.cls() as u8, frame.id.sid() as u8) };
        let endpoint = Endpoint::try_from(frame.id.endpoint()).unwrap();
        let payload =
            unsafe { core::slice::from_raw_parts(frame.buf.as_ptr(), frame.len as usize) };

        Frame {
            did,
            direction,
            kind,
            action,
            endpoint,
            payload,
        }
    }

    pub fn to_ll(&self) -> ll::caniot_frame_t {
        let mut frame: ll::caniot_frame_t = unsafe { core::mem::zeroed() };

        frame.id.set_cls(self.did.class as u32);
        frame.id.set_sid(self.did.sub_id as u32);
        frame.id.set_endpoint(self.endpoint as u32);
        frame.id.set_type(match (self.action, self.kind) {
            (Action::Write, Kind::Telemetry) => ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_COMMAND,
            (Action::Read, Kind::Telemetry) => ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_TELEMETRY,
            (Action::Read, Kind::Attribute) => {
                ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_READ_ATTRIBUTE
            }
            (Action::Write, Kind::Attribute) => {
                ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE
            }
        });
        frame.id.set_query(match self.direction {
            Direction::Query => ll::caniot_frame_dir_t::CANIOT_QUERY,
            Direction::Response => ll::caniot_frame_dir_t::CANIOT_RESPONSE,
        });

        frame.len = self.payload.len() as u8;
        frame.buf[..self.payload.len()].copy_from_slice(self.payload);

        frame
    }
}

impl<'a> Debug for Frame<'a> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Frame")
            .field("did", &self.did)
            .field("direction", &self.direction)
            .field("kind", &self.kind)
            .field("action", &self.action)
            .field("endpoint", &self.endpoint)
            .field("payload", &self.payload)
            .finish()
    }
}
