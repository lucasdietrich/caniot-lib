# CANIOT Protocol

*CANIoT* is a simple application protocol designed to be used on top of the CAN protocol.

It enables communication between up to 63 sensors and one or several controllers.

This includes control, telemetry and configuration.

---

## I. Introduction

A CAN message contains an arbitration ID (CAN ID) and a data field.
- The Arbitration ID can be 11bit (*standard*, `std`) or 29bit (*extended*, `ext`)
- The data field can be up to 8 bytes long

There is one more message type: remote request (`RTR`), which is used to request data from a remote node.

CANIOT is a protocol based on the CAN bus.


## II. Nodes

CANIOT protocol connects nodes using the CAN bus. A node can be either  a device or a controller.

### 1. Device

A device is a node which has sensors and/or actuators. (e.g. a temperature sensor, a heating controller).

Their role is to perform actions and report their state/measurements to the controller.

A device is identified by a unique ID which is the "*device id*" (`did`).

The device id is itself composed of the device *class* (`cls`) and the device *sub-id* (`sid` or `dev`).

A device have several *endpoints* (`ep`).

#### a. Device class `cls`

Device class is a 3-bit number which identifies the device type. 

Device class defines the data types that are supported by the device and the number of endpoints the device has.

The class also entirely determines the format of the payload for *commands* and *telemetry* messages.

| Val (10) | Bin | Hex | Symbol | Description | EP0 | EP1 | EP2 |
| -------- | --- | --- | ------ | ----------- | --- | --- | --- |
| 0        | 000 | 0   | C0     | CRT         | 8   | X   | X   |
| 1        | 001 | 1   | C1     |             |     |     |     |
| 2        | 010 | 2   | C2     |             |     |     |     |
| 3        | 011 | 3   | C3     |             |     |     |     |
| 4        | 100 | 4   | C4     |             |     |     |     |
| 5        | 101 | 5   | C5     |             |     |     |     |
| 6        | 110 | 6   | C6     |             |     |     |     |
| 7        | 111 | 7   | C7     |             |     |     |     |

#### b. Device sub-id `sid`

| Val (10) | Bin | Hex | Symbol | Description                    |
| -------- | --- | --- | ------ | ------------------------------ |
| 0        | 000 | 00  | D0     | highest priority device        |
| 1        | 001 | 01  | D1     |                                |
| 2        | 010 | 02  | D2     |                                |
| 3        | 011 | 03  | D3     |                                |
| 4        | 100 | 04  | D4     |                                |
| 5        | 101 | 05  | D5     |                                |
| 6        | 110 | 06  | D6     | lowest priority device         |
| 7        | 111 | 07  | D8     | BROADCAST to all class devices |

### 2. Controller

- A controller which control the behaviour of the devices and monitor the data. Controllers can work as gateway and allow to control the nodes from outside of the CAN bus. e.g. A smartphone controlling the heating controller.

## III. Messages types

The protocol implements following messages types that can be exchanged between the nodes:

| Val (10) | Bin | Hex | Symbol | Description     |
| -------- | --- | --- | ------ | --------------- |
| 0        | 00  | 0   | C      | Command         |
| 1        | 01  | 1   | T      | Telemetry       |
| 2        | 10  | 2   | R      | Write-attribute |
| 3        | 11  | 3   | W      | Read-attribute  |

Messages can be exchanged (for most of them) in two directions:

| Val (10) | Bin | Hex | Symbol | Description |
| -------- | --- | --- | ------ | ----------- |
| 0        | 0   | 0   | Q      | Query       |
| 1        | 1   | 1   | R      | Response    |

## IV. Protocol description

It uses arbitration ID of 11bit.

| priority | low   |   |   |   |   |   |   |   |   | high |
| -------- | ----- | - | - | - | - | - | - | - | - | ---- |
| id type  | STDID |   |   |   |   |   |   |   |   |      |
| bit      | 0     | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9    | 10 |
|          | T     | T | Q | C | C | C | D | D | D | E    | E |

## V. Others

### 1. Versionning

---

- Telemetry is periodically sent by the device for a single endpoint. The one configured in the configuration. Others endpoints should be explicitly requested by the controller. 