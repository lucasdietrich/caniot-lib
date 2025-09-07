// use serde::Serialize;
// use strum::{EnumIter, IntoEnumIterator};

// use crate::{
//     caniot::{ClCd, Payload, ProtocolError, Temperature, Ty, Xps},
//     utils::math::avg_of_slice_opt,
// };

// use super::traits::{Class, ClassCommandTrait, ClassTelemetryTrait, TempSensType};

// pub const CLASS1_IO_COUNT: usize = 19;

// #[derive(Debug, Default, Clone, Copy, PartialEq, Serialize)]
// pub struct Telemetry {
//     pub ios: [bool; CLASS1_IO_COUNT],

//     pub temp_in: Temperature,
//     pub temp_out: [Temperature; 3],
// }

// impl ClassTelemetryTrait for Telemetry {
//     fn get_temperature(&self, sensor: TempSensType) -> Option<f32> {
//         match sensor {
//             TempSensType::BoardSensor => self.temp_in.to_celsius(),
//             TempSensType::ExternalSensor(index) => self
//                 .temp_out
//                 .get(index as usize)
//                 .and_then(|t| t.to_celsius()),
//             TempSensType::AnyExternal => self.temp_out.iter().find_map(|t| t.to_celsius()),
//             TempSensType::AvgExternal => avg_of_slice_opt(&self.temp_out.map(|t| t.to_celsius())),
//             TempSensType::Any => self.temp_in.to_celsius(),
//         }
//     }
// }

// impl TryFrom<&Payload<Ty>> for Telemetry {
//     type Error = ProtocolError;

//     fn try_from(payload: &Payload<Ty>) -> Result<Self, ProtocolError> {
//         let payload = payload.as_ref();
//         if payload.len() >= 8 {
//             Ok(Telemetry {
//                 ios: [
//                     payload[0] & 0b0000_0001 != 0, // pc0
//                     payload[0] & 0b0000_0010 != 0, // pc1
//                     payload[0] & 0b0000_0100 != 0, // pc2
//                     payload[0] & 0b0000_1000 != 0, // pc3
//                     payload[0] & 0b0001_0000 != 0, // pd0
//                     payload[0] & 0b0010_0000 != 0, // pd1
//                     payload[0] & 0b0100_0000 != 0, // pd2
//                     payload[0] & 0b1000_0000 != 0, // pd3
//                     payload[1] & 0b0000_0001 != 0, // eio0
//                     payload[1] & 0b0000_0010 != 0, // eio1
//                     payload[1] & 0b0000_0100 != 0, // eio2
//                     payload[1] & 0b0000_1000 != 0, // eio3
//                     payload[1] & 0b0001_0000 != 0, // eio4
//                     payload[1] & 0b0010_0000 != 0, // eio5
//                     payload[1] & 0b0100_0000 != 0, // eio6
//                     payload[1] & 0b1000_0000 != 0, // eio7
//                     payload[2] & 0b0000_0001 != 0, // pb0
//                     payload[2] & 0b0000_0010 != 0, // pe0
//                     payload[2] & 0b0000_0100 != 0, // pe1
//                 ],
//                 temp_in: Temperature::from_raw_u10(u16::from_le_bytes([
//                     payload[3],
//                     payload[4] & 0b0000_0011,
//                 ])),
//                 temp_out: [
//                     Temperature::from_raw_u10(u16::from_le_bytes([
//                         (payload[4] >> 2) | (payload[5] & 0b0000_0011) << 6,
//                         (payload[5] & 0b0000_1100) >> 2,
//                     ])),
//                     Temperature::from_raw_u10(u16::from_le_bytes([
//                         (payload[5] >> 4) | (payload[6] & 0b0000_1111) << 4,
//                         (payload[6] & 0b0011_0000) >> 4,
//                     ])),
//                     Temperature::from_raw_u10(u16::from_le_bytes([
//                         (payload[6] >> 6) | (payload[7] & 0b0011_1111) << 2,
//                         (payload[7] & 0b1100_0000) >> 6,
//                     ])),
//                 ],
//             })
//         } else {
//             Err(ProtocolError::PayloadDecodeError)
//         }
//     }
// }

// impl Into<Payload<Ty>> for Telemetry {
//     fn into(self) -> Payload<Ty> {
//         let mut payload = Vec::with_capacity(8);

//         payload.push(
//             (self.ios[0] as u8)
//                 | (self.ios[1] as u8) << 1
//                 | (self.ios[2] as u8) << 2
//                 | (self.ios[3] as u8) << 3
//                 | (self.ios[4] as u8) << 4
//                 | (self.ios[5] as u8) << 5
//                 | (self.ios[6] as u8) << 6
//                 | (self.ios[7] as u8) << 7,
//         );
//         payload.push(
//             (self.ios[8] as u8)
//                 | (self.ios[9] as u8) << 1
//                 | (self.ios[10] as u8) << 2
//                 | (self.ios[11] as u8) << 3
//                 | (self.ios[12] as u8) << 4
//                 | (self.ios[13] as u8) << 5
//                 | (self.ios[14] as u8) << 6
//                 | (self.ios[15] as u8) << 7,
//         );
//         payload.push((self.ios[16] as u8) | (self.ios[17] as u8) << 1 | (self.ios[18] as u8) << 2);
//         let temp_in = self.temp_in.to_raw_u10_bytes();
//         let temp_out = [
//             self.temp_out[0].to_raw_u10_bytes(),
//             self.temp_out[1].to_raw_u10_bytes(),
//             self.temp_out[2].to_raw_u10_bytes(),
//         ];

//         payload.push(temp_in[0]);
//         payload.push(temp_in[1] | (temp_out[0][0] << 2));
//         payload.push(temp_out[0][0] >> 6 | (temp_out[0][1] << 2) | (temp_out[1][0] << 4));
//         payload.push(temp_out[1][0] >> 4 | (temp_out[1][1] << 4) | (temp_out[2][0] << 6));
//         payload.push(temp_out[2][0] >> 2 | (temp_out[2][1] << 6));

//         Payload::<Ty>::new(payload).unwrap()
//     }
// }

// #[repr(u8)]
// #[derive(EnumIter)]
// pub enum Class1CommandFields {
//     Poc0,
//     Poc1,
//     Poc2,
//     Poc3,
//     Pd0,
//     Pd1,
//     Pd2,
//     Pd3,
//     Eio0,
//     Eio1,
//     Eio2,
//     Eio3,
//     Eio4,
//     Eio5,
//     Eio6,
//     Eio7,
//     Pb0,
//     Ee0,
//     Ee1,
// }

// #[derive(Debug, Default, Clone, Copy, PartialEq, Serialize)]
// pub struct Command {
//     pub ios: [Xps; CLASS1_IO_COUNT],
// }

// impl<'a> ClassCommandTrait for Command {
//     fn has_effect(&self) -> bool {
//         self.ios.iter().any(|&x| x != Xps::None)
//     }
// }

// impl Into<Payload<ClCd>> for Command {
//     fn into(self) -> Payload<ClCd> {
//         let mut payload = vec![0; 7];
//         for (i, field) in self.ios.iter().enumerate() {
//             field.set_at(&mut payload, i).unwrap();
//         }

//         Payload::<ClCd>::new(payload).unwrap()
//     }
// }

// impl TryFrom<&Payload<ClCd>> for Command {
//     type Error = ProtocolError;

//     // Convert a Class1 command serialized payload into a Command struct
//     fn try_from(payload: &Payload<ClCd>) -> Result<Self, ProtocolError> {
//         let payload = payload.as_ref();
//         if payload.len() >= 7 {
//             Ok(Command {
//                 ios: Class1CommandFields::iter()
//                     .map(|field| Xps::get_at(payload, field as usize).unwrap_or_default())
//                     .collect::<Vec<Xps>>()
//                     .try_into()
//                     .unwrap(),
//             })
//         } else {
//             Err(ProtocolError::PayloadDecodeError)
//         }
//     }
// }

// pub struct Class1;

// impl Class for Class1 {
//     const CLASS_ID: u8 = 1;

//     type Telemetry = Telemetry;
//     type Command = Command;
// }
