use core::{fmt::Debug, time::Duration};

use caniot_sys as ll;

use crate::{controller::query::Handle, did::DeviceId, frame::FrameRef};

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ControllerEvent<'a> {
    /// Frame received that does not belong to a tracked query.
    Orphan {
        did: DeviceId,
        status: MessageStatus<'a>,
    },
    /// Frame received (or timeout/cancel) that belongs to a tracked query.
    Query {
        did: DeviceId,
        handle: Handle,
        terminated: bool,
        result: QueryResult<'a>,
        duration: Duration,
    },
}

impl<'a> ControllerEvent<'a> {
    pub fn get_did(&self) -> DeviceId {
        match self {
            ControllerEvent::Orphan { did, .. } => *did,
            ControllerEvent::Query { did, .. } => *did,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum MessageStatus<'a> {
    Ok { frame: FrameRef<'a> },
    Error { frame: FrameRef<'a> },
}

impl<'a> MessageStatus<'a> {
    pub fn frame(&self) -> &FrameRef<'a> {
        match self {
            MessageStatus::Ok { frame } | MessageStatus::Error { frame } => frame,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum QueryResult<'a> {
    Ok { payload: FrameRef<'a> },
    Error { payload: FrameRef<'a> },
    Timeout,
    Cancelled,
}

impl<'a> QueryResult<'a> {
    pub fn payload(&self) -> Option<&FrameRef<'a>> {
        match self {
            QueryResult::Ok { payload } | QueryResult::Error { payload } => Some(payload),
            QueryResult::Timeout | QueryResult::Cancelled => None,
        }
    }
}

impl<'a> ControllerEvent<'a> {
    pub(crate) unsafe fn from_ll_unchecked(inner: *const ll::caniot_controller_event_t) -> Self {
        unsafe {
            match (*inner).context() {
                ll::caniot_controller_event_context_t::CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN => {
                    let did = DeviceId::from_raw_unchecked((*inner).did);
                    let frame = FrameRef::from_ll_unchecked((*inner).response);
                    let kind = if (*inner).status()
                        == ll::caniot_controller_event_status_t::CANIOT_CONTROLLER_EVENT_STATUS_OK
                    {
                        MessageStatus::Ok { frame }
                    } else {
                        MessageStatus::Error { frame }
                    };
                    ControllerEvent::Orphan { did, status: kind }
                }
                ll::caniot_controller_event_context_t::CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY => {
                    let did = DeviceId::from_raw_unchecked((*inner).did);
                    let handle = Handle::new_unchecked((*inner).handle);
                    let terminated = (*inner).terminated() != 0;
                    let kind = match (*inner).status() {
                        ll::caniot_controller_event_status_t::CANIOT_CONTROLLER_EVENT_STATUS_OK => {
                            let payload = FrameRef::from_ll_unchecked((*inner).response);
                            QueryResult::Ok { payload }
                        }
                        ll::caniot_controller_event_status_t::CANIOT_CONTROLLER_EVENT_STATUS_ERROR => {
                            let payload = FrameRef::from_ll_unchecked((*inner).response);
                            QueryResult::Error { payload }
                        }
                        ll::caniot_controller_event_status_t::CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT => {
                            QueryResult::Timeout
                        }
                        ll::caniot_controller_event_status_t::CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED => {
                            QueryResult::Cancelled
                        }
                        _ => panic!("Unknown status"),
                    };
                    let duration = Duration::from_millis((*inner).duration as u64);
                    ControllerEvent::Query {
                        did,
                        handle,
                        terminated,
                        result: kind,
                        duration,
                    }
                }
                _ => panic!("Unknown event type"),
            }
        }
    }
}
