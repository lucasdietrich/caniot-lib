use caniot::{
    device::{Device, StaticConfig},
    did::DeviceId,
    driver::LinuxDriver,
    types::Endpoint,
};
use caniot_emu::{
    NodeApi, garage::GarageController, heaters::HeatersController,
    outdoor_alarm::OutdoorAlarmController, run_nodes,
};
use log::{debug, error, info, warn};

const DEFAULT_DEV: &str = "vcan0";

pub const DEVICE_DEMO_DID: u8 = 0;
pub const DEVICE_HEATERS_DID: u8 = 1;
pub const DEVICE_GARAGE_DID: u8 = 0x10;
pub const DEVICE_OUTDOOR_ALARM_DID: u8 = 0x18;

fn main() {
    let args = std::env::args().collect::<Vec<String>>();
    if args.len() > 1 {
        if args[1] == "--help" || args[1] == "-h" {
            println!("Usage: {} [dev]", args[0]);
            println!("  dev - CAN interface to use (default: {})", DEFAULT_DEV);
            return;
        } else {
            println!("Using CAN interface: {}", args[1]);
        }
    }
    let dev = args.get(1).map(|s| s.as_str()).unwrap_or(DEFAULT_DEV);
    info!("Using CAN interface: {}", dev);

    simple_logger::init_with_level(log::Level::Info).unwrap();

    let garage = GarageController::default();
    let garage_config = StaticConfig::new();
    let garage_driver = LinuxDriver::init(dev).expect("Failed to initialize Linux driver");

    let mut garage_node = Device::init(
        garage_driver,
        DeviceId::new(0, 2).unwrap(),
        garage,
        garage_config,
    )
    .expect("Failed to initialize garage device");
    garage_node.request_endpoint_telemetry(Endpoint::BoardControl);

    let heaters = HeatersController::default();
    let heaters_config = StaticConfig::new();
    let heaters_driver = LinuxDriver::init(dev).expect("Failed to initialize Linux driver");

    let mut heaters_node = Device::init(
        heaters_driver,
        DeviceId::new(1, 0).unwrap(),
        heaters,
        heaters_config,
    )
    .expect("Failed to initialize heaters device");

    let outdoor_alarm = OutdoorAlarmController::new();
    let outdoor_alarm_config = StaticConfig::new();
    let outdoor_alarm_driver = LinuxDriver::init(dev).expect("Failed to initialize Linux driver");

    let mut outdoor_alarm_node = Device::init(
        outdoor_alarm_driver,
        DeviceId::new(0, 3).unwrap(),
        outdoor_alarm,
        outdoor_alarm_config,
    )
    .expect("Failed to initialize outdoor alarm device");

    run_nodes(&mut [&mut garage_node, &mut heaters_node, &mut outdoor_alarm_node]);
}
