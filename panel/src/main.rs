use std::env;

use anyhow::Result;

mod packets;
mod parser;
mod serial;
mod tui;

fn main() -> Result<()> {
    let port = env::args().nth(1).unwrap_or("/dev/ttyUSB0".to_string());

    let ser = serial::Serial::new(&port).unwrap();
    tui::start(ser)
}
