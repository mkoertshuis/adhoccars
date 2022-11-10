use nom::{
    branch::alt,
    bytes::complete::tag,
    multi::many1,
    number::streaming::{be_u64, le_i8, le_u8},
    sequence::{preceded, tuple},
    IResult,
};

use crate::packets::{Packet, MAC, RSSI};

type NomResult<'a, T> = IResult<&'a [u8], T>;

pub fn parse(input: &[u8]) -> NomResult<Vec<Packet>> {
    many1(alt((parse_kill, parse_rssi, parse_devices)))(input)
}

fn parse_kill(input: &[u8]) -> NomResult<Packet> {
    let (rem, _) = tag(&[0x03])(input)?;

    Ok((rem, Packet::Kill))
}

fn parse_rssi(input: &[u8]) -> NomResult<Packet> {
    let (rem, (_, mac, rssi)) = tuple((tag(&[0x04]), parse_mac_value, parse_rssi_value))(input)?;

    Ok((rem, Packet::RSSI(mac, rssi)))
}

fn parse_devices(input: &[u8]) -> NomResult<Packet> {
    let (rem, n) = preceded(tag(&[0x10]), parse_devices_value)(input)?;

    Ok((rem, Packet::Devices(n)))
}

fn parse_mac_value(input: &[u8]) -> NomResult<MAC> {
    be_u64(input)
}

fn parse_rssi_value(input: &[u8]) -> NomResult<RSSI> {
    le_i8(input)
}

fn parse_devices_value(input: &[u8]) -> NomResult<u8> {
    le_u8(input)
}
