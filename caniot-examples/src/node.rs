use std::{
    os::fd::AsFd,
    time::{Duration, Instant},
};

use caniot::{
    Driver,
    device::{Device, implementation::DeviceApi},
    error::FailCode,
    types::Endpoint,
};
use log::{debug, error};
use nix::poll::{PollFd, PollFlags, PollTimeout, poll};

pub trait NodeApi {
    // Return the duration until the next action is required, or None if no action is required.
    fn app_next_timeout(&self, _now: &Instant) -> Option<Duration> {
        None
    }

    // Process any pending actions. Return the endpoint that needs to be notified, if any.
    fn app_process(&mut self, _now: &Instant) -> Option<Endpoint> {
        None
    }
}

pub trait Node: AsFd {
    // Return the duration until the next action is required, or None if no action is required.
    fn next_timeout(&mut self, now: &Instant) -> Option<Duration>;

    // Process any pending actions. Return the endpoint that needs to be notified, if any.
    fn process(&mut self, now: &Instant);
}

impl<D: Driver + AsFd, A: DeviceApi + NodeApi + 'static> Node for Device<D, A> {
    fn next_timeout(&mut self, now: &Instant) -> Option<Duration> {
        [
            Device::<D, A>::next_timeout(self),
            self.get_api_mut().app_next_timeout(now),
        ]
        .into_iter()
        .flatten()
        .min()
    }

    fn process(&mut self, now: &Instant) {
        match self.run_once() {
            Ok(()) => {}
            Err(FailCode::EAGAIN) => {
                log::warn!("Device error: {}", FailCode::EAGAIN);
            }
            Err(e) => {
                error!("Device error: {}", e);
            }
        }

        if let Some(requested_ep) = self.get_api_mut().app_process(now) {
            self.request_endpoint_telemetry(requested_ep);
        }
    }
}

pub fn run_nodes(nodes: &mut [&mut dyn Node]) {
    loop {
        let now = Instant::now();
        let timeout = nodes
            .iter_mut()
            .filter_map(|node| node.next_timeout(&now))
            .min()
            .map(|d| {
                d.try_into()
                    .expect("Failed to convert duration to PollTimeout")
            })
            .unwrap_or(PollTimeout::NONE);
        let mut fds = nodes
            .iter()
            .map(|node| PollFd::new(node.as_fd(), PollFlags::POLLIN))
            .collect::<Vec<_>>();
        debug!("Polling {:?} with timeout {:?}", fds, timeout);
        match poll(&mut fds, timeout) {
            Ok(_n) => {
                let now = Instant::now();

                for node in nodes.iter_mut() {
                    node.process(&now);
                }
            }
            Err(err) => {
                error!("Poll error: {}", err);
            }
        };
    }
}
