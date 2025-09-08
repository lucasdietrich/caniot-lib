use std::time::{Duration, Instant};

use caniot::datatypes::Xps;

use super::helpers::*;

#[test]
fn test_xps_simple() {
    let now = Instant::now();
    let mut xps = EmuXps::new(false, false, None);
    assert_eq!(xps.get_state(), false);
    assert_eq!(xps.supports_pulse(), false);
    assert_eq!(xps.pulse_pending(), false);
    assert_eq!(xps.pulse_expired(&now), false);
    assert_eq!(xps.time_to_pulse_expire(&now), None);
    assert_eq!(xps.pulse_process(&now), None);

    xps.apply(&Xps::SetOn);
    assert_eq!(xps.get_state(), true);

    xps.apply(&Xps::SetOff);
    assert_eq!(xps.get_state(), false);

    xps.apply(&Xps::SetOn);
    assert_eq!(xps.get_state(), true);

    xps.apply(&Xps::None);
    assert_eq!(xps.get_state(), true);

    xps.apply(&Xps::Reset);
    assert_eq!(xps.get_state(), false);
}

#[test]
fn test_xps_pulse() {
    let now = Instant::now();
    let duration = Duration::from_millis(100);
    let mut xps = EmuXps::new(false, false, Some(duration));

    assert_eq!(xps.get_state(), false);
    assert_eq!(xps.supports_pulse(), true);
    assert_eq!(xps.pulse_pending(), false);
    assert_eq!(xps.pulse_expired(&now), false);
    assert_eq!(xps.time_to_pulse_expire(&now), None);
    assert_eq!(xps.pulse_process(&now), None);

    xps.apply(&Xps::PulseOn);
    assert_eq!(xps.get_state(), true);
    assert_eq!(xps.pulse_pending(), true);
    assert_eq!(xps.pulse_expired(&now), false);

    // This sleep is bad (todo, create an "advance()" method to manually advance in time)
    std::thread::sleep(duration);

    assert_eq!(xps.pulse_expired(&now), true);
    assert_eq!(xps.pulse_process(&now), Some(false));
    assert_eq!(xps.get_state(), false);
    assert_eq!(xps.pulse_pending(), false);
    assert_eq!(xps.pulse_expired(&now), false);
    assert_eq!(xps.time_to_pulse_expire(&now), None);
}
