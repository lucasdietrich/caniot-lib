use crate::controller::event::ControllerEvent;

pub struct DummyControllerApi;

impl ControllerApi for DummyControllerApi {}

pub enum EventVerdict {
    Continue,
    Abort,
}

pub trait ControllerApi {
    fn on_event(&mut self, _event: ControllerEvent) -> EventVerdict {
        EventVerdict::Continue
    }
}
