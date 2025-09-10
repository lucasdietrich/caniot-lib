use core::fmt;

use caniot_sys as ll;

use num_derive::FromPrimitive;

#[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
pub enum Type {
    Telemetry = 0,
    Command = 1,
    ReadAttribute = 2,
    WriteAttribute = 3,
}

impl Type {
    pub fn get_action(&self) -> Action {
        match self {
            Type::Telemetry => Action::Read,
            Type::Command => Action::Write,
            Type::ReadAttribute => Action::Read,
            Type::WriteAttribute => Action::Write,
        }
    }
}

impl From<Type> for ll::caniot_frame_type_t::Type {
    fn from(kind: Type) -> Self {
        match kind {
            Type::Telemetry => ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_TELEMETRY,
            Type::Command => ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_COMMAND,
            Type::ReadAttribute => ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
            Type::WriteAttribute => ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE,
        }
    }
}

impl From<ll::caniot_frame_type_t::Type> for Type {
    fn from(value: ll::caniot_frame_type_t::Type) -> Self {
        match value {
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_TELEMETRY => Type::Telemetry,
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_COMMAND => Type::Command,
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_READ_ATTRIBUTE => Type::ReadAttribute,
            ll::caniot_frame_type_t::CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE => Type::WriteAttribute,
            _ => panic!("Unknown frame type"),
        }
    }
}

// #[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
// pub enum Kind {
//     Telemetry = 0,
//     Attribute = 1,
// }

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

impl From<Direction> for ll::caniot_frame_dir_t::Type {
    fn from(direction: Direction) -> Self {
        match direction {
            Direction::Query => ll::caniot_frame_dir_t::CANIOT_QUERY,
            Direction::Response => ll::caniot_frame_dir_t::CANIOT_RESPONSE,
        }
    }
}

impl From<ll::caniot_frame_dir_t::Type> for Direction {
    fn from(value: ll::caniot_frame_dir_t::Type) -> Self {
        match value {
            ll::caniot_frame_dir_t::CANIOT_QUERY => Direction::Query,
            ll::caniot_frame_dir_t::CANIOT_RESPONSE => Direction::Response,
            _ => panic!("Unknown frame direction"),
        }
    }
}

#[derive(Debug, PartialEq, Eq, Clone, Copy, FromPrimitive)]
pub enum Endpoint {
    ApplicationDefault = 0,
    Application1 = 1,
    Application2 = 2,
    BoardControl = 3,
}

impl From<Endpoint> for u8 {
    fn from(endpoint: Endpoint) -> Self {
        endpoint as u8
    }
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