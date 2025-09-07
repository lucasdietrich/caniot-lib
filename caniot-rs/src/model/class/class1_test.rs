// use num::FromPrimitive;

// use crate::caniot::{AsPayload, Payload, Temperature, Ty};

// use super::class1;

// #[test]
// fn telemetry() {
//     let telem = class1::Telemetry::default();
//     let ser = telem.to_raw_vec();
//     assert_eq!(ser.len(), 8);
//     assert_eq!(ser, vec![0, 0, 0, 255, 255, 255, 255, 255]);

//     for i in 0..class1::Telemetry::default().ios.len() {
//         let mut telem = class1::Telemetry::default();
//         telem.ios[i] = true;
//         let ser = telem.to_raw_vec();
//         assert_eq!(ser.len(), 8);
//         assert_eq!(ser[i / 8], 1 << (i % 8));
//     }

//     let temps = [
//         Temperature::new(-1100),
//         Temperature::new(0),
//         Temperature::new(3300),
//         Temperature::new(4400),
//     ];

//     for i in 0..=2 {
//         for temp in temps.iter() {
//             let mut telem = class1::Telemetry::default();
//             telem.temp_out[i] = *temp;
//             let ser = telem.to_raw_vec();

//             let pl = Payload::<Ty>::try_from(ser).unwrap();
//             let deser = class1::Telemetry::try_from(&pl);
//             assert_eq!(deser.is_ok(), true);

//             let deser = deser.unwrap();
//             assert_eq!(deser.temp_out[i], *temp);
//         }
//     }
// }

// #[test]
// fn command() {
//     let mut cmd = class1::Command::default();
//     for i in 0..class1::CLASS1_IO_COUNT {
//         FromPrimitive::from_u8((i & 0x7) as u8).map(|x| cmd.ios[i] = x);
//     }

//     let ser = cmd.to_raw_vec();
//     assert_eq!(ser.len(), 7);

//     let deser = class1::Command::try_from_raw(&ser);
//     assert_eq!(deser.is_ok(), true);

//     let deser = deser.unwrap();
//     for i in 0..class1::CLASS1_IO_COUNT {
//         assert_eq!(deser.ios[i], cmd.ios[i]);
//     }
// }
