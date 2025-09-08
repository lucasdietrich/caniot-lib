use caniot::{
    device::{Device, StaticConfig},
    did::DeviceId,
    driver::LinuxDriver,
    types::Endpoint,
};
use caniot_emu::{garage::GarageController, heaters::HeatersController, outdoor_alarm::OutdoorAlarmController, run_nodes, NodeApi};
use log::{debug, error, info, warn};

const DEV: &str = "vcan0";

pub const DEVICE_DEMO_DID: u8 = 0;
pub const DEVICE_HEATERS_DID: u8 = 1;
pub const DEVICE_GARAGE_DID: u8 = 0x10;
pub const DEVICE_OUTDOOR_ALARM_DID: u8 = 0x18;

fn main() {
    simple_logger::init_with_level(log::Level::Info).unwrap();

    let garage = GarageController::default();
    let garage_config = StaticConfig::new();
    let garage_driver = LinuxDriver::init(DEV).expect("Failed to initialize Linux driver");

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
    let heaters_driver = LinuxDriver::init(DEV).expect("Failed to initialize Linux driver");

    let mut heaters_node = Device::init(
        heaters_driver,
        DeviceId::new(1, 0).unwrap(),
        heaters,
        heaters_config,
    )
    .expect("Failed to initialize heaters device");

    let outdoor_alarm = OutdoorAlarmController::new();
    let outdoor_alarm_config = StaticConfig::new();
    let outdoor_alarm_driver = LinuxDriver::init(DEV).expect("Failed to initialize Linux driver");

    let mut outdoor_alarm_node = Device::init(
        outdoor_alarm_driver,
        DeviceId::new(0, 3).unwrap(),
        outdoor_alarm,
        outdoor_alarm_config,
    )
    .expect("Failed to initialize outdoor alarm device");

    run_nodes(&mut [&mut garage_node, &mut heaters_node, &mut outdoor_alarm_node]);
}
