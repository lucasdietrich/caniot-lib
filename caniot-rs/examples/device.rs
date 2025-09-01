use std::{os::fd::AsFd, vec};

use caniot::{
    device::{
        Device, StaticConfig,
        implementation::{DeviceApi, TelemetryError},
    },
    driver::LinuxDriver,
    types::Endpoint,
};
use log::{debug, error, info, warn};
use nix::poll::{PollFd, PollFlags, PollTimeout, poll};

pub struct Sensor;

impl DeviceApi for Sensor {
    fn telemetry(&mut self, ep: Endpoint) -> Result<Vec<u8>, TelemetryError> {
        info!("Sensor telemetry request for endpoint {:?}", ep);

        match ep {
            Endpoint::ApplicationDefault => Ok(vec![0x01, 0x02, 0x03]),
            _ => Err(TelemetryError::NotSupported),
        }
    }

    fn command(&mut self, ep: Endpoint, data: &[u8]) -> Result<(), TelemetryError> {
        info!(
            "Sensor command received for endpoint {:?} with data {:?}",
            ep, data
        );
        Ok(())
    }
}

const DEV: &str = "vcan0";

fn main() {
    simple_logger::init_with_level(log::Level::Info).unwrap();

    let sensor1 = Sensor;
    let config1 = StaticConfig::new();
    let driver1 = LinuxDriver::init(DEV).expect("Failed to initialize Linux driver");

    let mut device1 =
        Device::init(driver1, 8, sensor1, config1).expect("Failed to initialize device");
    device1.request_endpoint_telemetry(Endpoint::ApplicationDefault);

    let sensor2 = Sensor;
    let config2 = StaticConfig::new();
    let driver2 = LinuxDriver::init(DEV).expect("Failed to initialize Linux driver");

    let mut device2 =
        Device::init(driver2, 10, sensor2, config2).expect("Failed to initialize device");
    device2.request_endpoint_telemetry(Endpoint::ApplicationDefault);

    let handle = std::thread::spawn(move || {
        loop {
            let timeout = [device1.next_timeout(), device2.next_timeout()]
                .into_iter()
                .flatten()
                .min()
                .map(|d| {
                    d.try_into()
                        .expect("Failed to convert duration to PollTimeout")
                })
                .unwrap_or(PollTimeout::NONE);
            let pollfd1 = PollFd::new(device1.as_fd(), PollFlags::POLLIN);
            let pollfd2 = PollFd::new(device2.as_fd(), PollFlags::POLLIN);
            let mut fds = [pollfd1, pollfd2];
            debug!("Polling {:?} with timeout {:?}", fds, timeout);
            match poll(&mut fds, timeout) {
                Ok(n) => {
                    let fd1_ready = fds[0]
                        .revents()
                        .unwrap_or(PollFlags::empty())
                        .contains(PollFlags::POLLIN);
                    let fd2_ready = fds[1]
                        .revents()
                        .unwrap_or(PollFlags::empty())
                        .contains(PollFlags::POLLIN);

                    if fd1_ready || n == 0 {
                        match device1.run_once() {
                            Ok(()) => {}
                            Err(err) => {
                                if !err.is_eagain() {
                                    error!("Device 1 error: {}", err);
                                } else {
                                    warn!("Device 1 non-fatal error: {}", err);
                                }
                            }
                        }
                    }
                    if fd2_ready || n == 0 {
                        match device2.run_once() {
                            Ok(()) => {}
                            Err(err) => {
                                if !err.is_eagain() {
                                    error!("Device 1 error: {}", err);
                                } else {
                                    warn!("Device 1 non-fatal error: {}", err);
                                }
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
