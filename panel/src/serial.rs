use std::{
    io::{Read, Write},
    time::Duration,
};

use anyhow::{anyhow, Result};
use serial::{unix::TTYPort, SerialPort};

use crate::{packets::Packet, parser::parse};

const SETTINGS: serial::PortSettings = serial::PortSettings {
    baud_rate: serial::Baud115200,
    char_size: serial::Bits8,
    parity: serial::ParityNone,
    stop_bits: serial::Stop1,
    flow_control: serial::FlowNone,
};

pub struct Serial {
    port: TTYPort,
    buf: Vec<u8>,
}

impl Serial {
    pub fn new(addr: &str) -> Result<Self> {
        let mut port = serial::open(addr).expect("Failed to connect to serial!");
        port.configure(&SETTINGS)?;
        port.set_timeout(Duration::from_secs(1))?;

        Ok(Self {
            port,
            buf: Vec::new(),
        })
    }

    pub fn parse(&mut self) -> Result<Vec<Packet>> {
        let mut buf = [0; 256];
        let n = self.port.read(&mut buf)?;
        self.buf.append(&mut buf[..n].to_owned());

        match parse(&self.buf.clone()) {
            Ok((r, p)) => {
                self.buf.clear();
                self.buf.append(&mut r.to_owned());

                Ok(p)
            }
            Err(nom::Err::Incomplete(_)) => {
                return Ok(Vec::new());
            }
            Err(e) => return Err(anyhow!(e.to_string())),
        }
    }

    pub fn send(&mut self, packet: Packet) -> Result<usize> {
        self.port
            .write(&packet.encode())
            .map_err(|err| anyhow!(err))
    }
}
