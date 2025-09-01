use core::fmt;

use caniot_sys as ll;

use num::FromPrimitive;
use num_derive::FromPrimitive;

use crate::did::DeviceId;

#[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
pub enum Kind {
    Telemetry = 0,
    Attribute = 1,
}

#[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
pub enum Action {
    Write = 0,
    Read = 1,
}

#[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
pub enum Direction {
    Query = 0,
    Response = 1,
}

#[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
pub enum Endpoint {
    ApplicationDefault = 0,
    Application1 = 1,
    Application2 = 2,
    BoardControl = 3,
}

impl From<Endpoint> for ll::caniot_endpoint_t::Type {
    fn from(endpoint: Endpoint) -> Self {
        match endpoint {
            Endpoint::ApplicationDefault => ll::caniot_endpoint_t::CANIOT_ENDPOINT_APP,
            Endpoint::Application1 => ll::caniot_endpoint_t::CANIOT_ENDPOINT_1,
            Endpoint::Application2 => ll::caniot_endpoint_t::CANIOT_ENDPOINT_2,
            Endpoint::BoardControl => ll::caniot_endpoint_t::CANIOT_ENDPOINT_BOARD_CONTROL,
        }
    }
}

impl TryFrom<ll::caniot_endpoint_t::Type> for Endpoint {
    type Error = ();

    fn try_from(value: ll::caniot_endpoint_t::Type) -> Result<Self, Self::Error> {
        match value {
            ll::caniot_endpoint_t::CANIOT_ENDPOINT_APP => Ok(Endpoint::ApplicationDefault),
            ll::caniot_endpoint_t::CANIOT_ENDPOINT_1 => Ok(Endpoint::Application1),
            ll::caniot_endpoint_t::CANIOT_ENDPOINT_2 => Ok(Endpoint::Application2),
            ll::caniot_endpoint_t::CANIOT_ENDPOINT_BOARD_CONTROL => Ok(Endpoint::BoardControl),
            _ => Err(()),
        }
    }
}

impl fmt::Display for Endpoint {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Endpoint::ApplicationDefault => write!(f, "ep-0"),
            Endpoint::Application1 => write!(f, "ep-1"),
            Endpoint::Application2 => write!(f, "ep-2"),
            Endpoint::BoardControl => write!(f, "ep-c"),
        }
    }
}

#[derive(Clone, Copy)]
pub struct Id {
    pub(crate) device_id: DeviceId,
    pub(crate) direction: Direction,
    pub(crate) msg_type: Kind,
    pub(crate) action: Action,
    pub(crate) endpoint: Endpoint,
}

impl From<u16> for Id {
    fn from(id: u16) -> Self {
        Id {
            device_id: DeviceId::try_from(((id >> 3) & 0x3f) as u8).unwrap(),
            action: Action::from_u8((id & 0x1) as u8).unwrap(),
            msg_type: Kind::from_u8(((id >> 1) & 0x1) as u8).unwrap(),
            direction: Direction::from_u8(((id >> 2) & 0x1) as u8).unwrap(),
            endpoint: Endpoint::from_u8(((id >> 9) & 0x3) as u8).unwrap(),
        }
    }
}

impl Id {
    // Direct conversion functions instead of Into traits
    pub fn to_u16(self) -> u16 {
        let mut id: u16 = 0;
        id |= (self.device_id.class as u16) << 3;
        id |= (self.device_id.sub_id as u16) << 6;
        id |= self.action as u16;
        id |= (self.msg_type as u16) << 1;
        id |= (self.direction as u16) << 2;
        id |= (self.endpoint as u16) << 9;
        id
    }

    /// Returns the endpoint if the message is a telemetry message
    /// Returns None if the message is not a attribute message
    pub fn get_endpoint(&self) -> Option<Endpoint> {
        if self.msg_type == Kind::Telemetry {
            Some(self.endpoint)
        } else {
            None
        }
    }
}
