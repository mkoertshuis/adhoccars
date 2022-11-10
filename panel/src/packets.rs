#[derive(Debug, Clone, Copy)]
pub enum Packet {
    Control(ControlDirection),
    Leader(MAC),
    Kill,
    RSSI(MAC, RSSI),
    Devices(u8),
}

impl Packet {
    pub fn encode(&self) -> Vec<u8> {
        match self {
            Packet::Control(dir) => vec![0x01, dir.encode()],
            Packet::Leader(mac) => {
                let b = mac.to_be_bytes();
                vec![0x02, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]]
            }
            Packet::Kill => vec![0x03],
            Packet::Devices(n) => vec![0x10, *n],
            _ => unimplemented!(),
        }
    }
}

pub type MAC = u64;
pub type RSSI = i8;

#[derive(Debug, Clone, Copy)]
pub enum ControlDirection {
    Forward,
    Right,
    Backwards,
    Left,
    Break,
}

impl ControlDirection {
    pub fn encode(&self) -> u8 {
        match self {
            ControlDirection::Forward => 0x01,
            ControlDirection::Backwards => 0x02,
            ControlDirection::Left => 0x03,
            ControlDirection::Right => 0x04,
            ControlDirection::Break => 0x05,
        }
    }
}
