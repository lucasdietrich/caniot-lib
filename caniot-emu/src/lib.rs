use std::time::{Duration, Instant};

use caniot::{device::implementation::DeviceApi, types::Endpoint};

pub mod garage;

pub trait Node: DeviceApi {
    // Return the duration until the next action is required, or None if no action is required.
    fn next_timeout(&self, _now: &Instant) -> Option<Duration> {
        None
    }

    // Process any pending actions. Return the endpoint that needs to be notified, if any.
    fn process(&mut self, _now: &Instant) -> Option<Endpoint> {
        None
    }
}

// pub fn run_nodes(nodes: &mut [Box<dyn Node>]) {
//     loop {
//         let timeout = [device1.next_timeout()]
//             .into_iter()
//             .flatten()
//             .min()
//             .map(|d| {
//                 d.try_into()
//                     .expect("Failed to convert duration to PollTimeout")
//             })
//             .unwrap_or(PollTimeout::NONE);
//         let pollfd1 = PollFd::new(device1.as_fd(), PollFlags::POLLIN);
//         let mut fds = [pollfd1];
//         debug!("Polling {:?} with timeout {:?}", fds, timeout);
//         match poll(&mut fds, timeout) {
//             Ok(n) => {
//                 let fd1_ready = fds[0]
//                     .revents()
//                     .unwrap_or(PollFlags::empty())
//                     .contains(PollFlags::POLLIN);

//                 if fd1_ready || n == 0 {
//                     match device1.run_once() {
//                         Ok(()) => {}
//                         Err(err) => {
//                             if !err.is_eagain() {
//                                 error!("Device 1 error: {}", err);
//                             } else {
//                                 warn!("Device 1 non-fatal error: {}", err);
//                             }
//                         }
//                     }
//                 }
//             }
//             Err(err) => {
//                 error!("Poll error: {}", err);
//             }
//         };
//     }
// }
