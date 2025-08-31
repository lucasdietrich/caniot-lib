use std::{io::Read, os::fd::AsFd, time::Duration};

use caniot::{
    controller::{
        Controller,
        event::ControllerEvent,
        implementation::{ControllerApi, EventVerdict},
    },
    did::DeviceId,
    driver::LinuxDriver,
    frame::Frame,
};
use log::{debug, error, info};
use nix::poll::{PollFd, PollFlags, PollTimeout, poll};

struct MyController;

impl ControllerApi for MyController {
    fn on_event(&mut self, event: ControllerEvent) -> EventVerdict {
        info!("Controller event: {:#?}", event);
        EventVerdict::Continue
    }
}

fn main() {
    simple_logger::init_with_level(log::Level::Info).unwrap();

    let myctrl = MyController;
    let driver = LinuxDriver::init("vcan0").expect("Failed to initialize Linux driver");
    let mut ctrl = Controller::init(driver, myctrl).expect("Failed to initialize controller");

    let mut stdin = std::io::stdin();

    let handle = std::thread::spawn(move || {
        debug!("Controller next timeout: {:?}", ctrl.next_timeout());
        loop {
            let timeout = ctrl
                .next_timeout()
                .map(|d| {
                    d.try_into()
                        .expect("Failed to convert duration to PollTimeout")
                })
                .unwrap_or(PollTimeout::NONE);
            let pollfd1 = PollFd::new(ctrl.as_fd(), PollFlags::POLLIN);
            let pollfd2 = PollFd::new(AsFd::as_fd(&stdin), PollFlags::POLLIN);
            let mut fds = [pollfd1, pollfd2];
            debug!("Polling {:?} with timeout {:?}", fds, timeout);
            match poll(&mut fds, timeout) {
                Ok(n) => {
                    debug!("Poll returned {} fds: {:?}", n, fds);
                    let fd1_ready = fds[0]
                        .revents()
                        .unwrap_or(PollFlags::empty())
                        .contains(PollFlags::POLLIN);
                    let fd2_ready = fds[1]
                        .revents()
                        .unwrap_or(PollFlags::empty())
                        .contains(PollFlags::POLLIN);

                    if fd1_ready || n == 0 {
                        match ctrl.run() {
                            Ok(()) => {}
                            Err(err) => {
                                error!("Controller error: {}", err);
                            }
                        };
                    }

                    if fd2_ready {
                        let mut buf = [0u8; 1];
                        match stdin.read(&mut buf) {
                            Ok(0) => {
                                info!("Stdin closed");
                                break;
                            }
                            Ok(_) => match buf[0] {
                                b'q' | b'Q' => {
                                    info!("Query");
                                    // let did =
                                    //     DeviceId::try_from(8).expect("Failed to create DeviceId");
                                    let did = DeviceId::BROADCAST;
                                    let frame = Frame::new(did, &[1, 2, 3, 4])
                                        .expect("Failed to create frame");
                                    match ctrl
                                        .query(did, frame, Some(Duration::from_millis(1000))) {
                                        Ok(Some(handle)) => {
                                            info!("Query sent, handle: {:?}", handle);
                                        }
                                        Ok(None) => {
                                            info!("Query sent, no handle");
                                        }
                                        Err(err) => {
                                            error!("Failed to query device: {}", err);
                                        }
                                    }
                                }
                                _ => {
                                    info!("Pressed key: {:02x}", buf[0]);
                                }
                            },
                            Err(err) => {
                                error!("Stdin read error: {}", err);
                            }
                        }
                    }
                }
                Err(err) => {
                    error!("Poll error: {}", err);
                }
            };
        }
    });

    handle.join().expect("Thread panicked");
}
