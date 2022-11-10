use std::{
    collections::HashMap,
    io::stdout,
    time::{Duration, Instant},
};

use anyhow::Result;
use crossterm::{
    event::{self, DisableMouseCapture, EnableMouseCapture, Event, KeyCode},
    execute,
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
};
use tui::{
    backend::{Backend, CrosstermBackend},
    layout::{Alignment, Constraint, Direction, Layout},
    style::{Color, Modifier, Style},
    symbols,
    text::{Span, Spans},
    widgets::{
        Axis, Block, BorderType, Borders, Chart, Dataset, GraphType, List, ListItem, ListState,
        Paragraph,
    },
    Frame, Terminal,
};

use crate::{
    packets::{ControlDirection, Packet, MAC, RSSI},
    serial::Serial,
};

#[derive(PartialEq)]
enum SelectOp {
    UP,
    DOWN,
    MARK,
}

#[derive(Clone)]
struct Node {
    mac: MAC,
    rssi: Vec<(Instant, RSSI)>,
    epoch: Instant,
    leader: bool,
}

impl Node {
    pub fn new(mac: MAC, epoch: Instant, rssi: RSSI) -> Self {
        Self {
            mac,
            epoch,
            rssi: vec![(Instant::now(), rssi)],
            leader: false,
        }
    }

    pub fn get_data(&self) -> Vec<(f64, f64)> {
        self.rssi
            .iter()
            .map(|(time, rssi)| (time.duration_since(self.epoch).as_secs_f64(), *rssi as f64))
            .collect()
    }

    pub fn add_rssi(&mut self, rssi: RSSI) {
        self.rssi.push((Instant::now(), rssi))
    }
}

struct App {
    epoch: Instant,
    last_control: Option<Instant>,
    serial: Serial,
    selected: ListState,
    nodes: Vec<Node>,
    data: HashMap<MAC, Vec<(f64, f64)>>,
}

impl App {
    fn new(serial: Serial) -> App {
        let epoch = Instant::now();

        App {
            serial,
            epoch,
            last_control: None,
            selected: ListState::default(),
            nodes: Vec::new(),
            data: HashMap::new(),
        }
    }

    fn on_tick(&mut self) -> Result<()> {
        for node in &self.nodes {
            self.data.insert(node.mac, node.get_data());
        }

        self.nodes.retain(|node| {
            let last_seen = node
                .rssi
                .iter()
                .fold(node.rssi[0].0, |best, (i, _)| best.max(*i));

            Duration::from_secs(30) > last_seen.elapsed()
        });

        if self.selected.selected().is_none() {
            if self.nodes.len() > 0 {
                self.selected.select(Some(0));
            }
        } else {
            if self.nodes.len() == 0 {
                self.selected.select(None);
            }
        }

        if let Some(last) = self.last_control {
            if last.elapsed() > Duration::from_millis(1000) {
                self.last_control = None;
                self.serial.send(Packet::Control(ControlDirection::Break))?;
            }
        }

        for packet in self.serial.parse().unwrap() {
            match packet {
                Packet::RSSI(mac, rssi) => self.add_rssi(mac, rssi),
                _ => (),
            }
        }

        Ok(())
    }

    fn add_rssi(&mut self, mac: MAC, rssi: RSSI) {
        let found = self.nodes.iter_mut().find(|n| n.mac == mac);

        if let Some(n) = found {
            n.add_rssi(rssi);
        } else {
            let n = Node::new(mac, self.epoch, rssi);
            self.data.insert(mac, Vec::new());
            self.nodes.push(n)
        }
    }

    fn kill(&mut self) {
        self.serial.send(Packet::Kill).unwrap();
    }

    fn control(&mut self, dir: ControlDirection) {
        self.serial.send(Packet::Control(dir)).unwrap();
        self.last_control = Some(Instant::now());
    }

    fn select(&mut self, op: SelectOp) -> Result<()> {
        let i = if let Some(i) = self.selected.selected() {
            i as i64
        } else {
            return Ok(());
        };

        if op == SelectOp::MARK {
            for node in self.nodes.iter_mut() {
                node.leader = false;
            }

            let node = &mut self.nodes[i as usize];
            node.leader = true;

            self.serial.send(Packet::Leader(node.mac.clone()))?;
        }

        let mut next = match op {
            SelectOp::UP => i - 1,
            SelectOp::DOWN => i + 1,
            SelectOp::MARK => i,
        };

        if next < 0 {
            next = self.nodes.len() as i64 - 1;
        }

        if next >= self.nodes.len() as i64 {
            next = 0;
        }

        self.selected.select(Some(next as usize));
        Ok(())
    }

    pub fn get_datasets<'a>(&'a mut self) -> Vec<Dataset<'a>> {
        self.nodes
            .clone()
            .into_iter()
            .map(|node| {
                Dataset::default()
                    .name(format!("{:X}", node.mac))
                    .marker(symbols::Marker::Dot)
                    .style(
                        Style::default()
                            .fg(Color::Indexed(((node.mac % 6) + 9).try_into().unwrap())),
                    )
                    .graph_type(GraphType::Line)
                    .data(&self.data[&node.mac])
            })
            .collect()
    }
}

pub fn start(serial: Serial) -> Result<()> {
    // setup terminal
    enable_raw_mode()?;
    let mut stdout = stdout();
    execute!(stdout, EnterAlternateScreen, EnableMouseCapture)?;
    let backend = CrosstermBackend::new(stdout);
    let mut terminal = Terminal::new(backend)?;

    // create app and run it
    let tick_rate = Duration::from_millis(1000 / 60);
    let app = App::new(serial);
    let res = run_app(&mut terminal, app, tick_rate);

    // restore terminal
    disable_raw_mode()?;
    execute!(
        terminal.backend_mut(),
        LeaveAlternateScreen,
        DisableMouseCapture
    )?;
    terminal.show_cursor()?;

    if let Err(err) = res {
        panic!("{:?}", err)
    }

    Ok(())
}

fn run_app<B: Backend>(
    terminal: &mut Terminal<B>,
    mut app: App,
    tick_rate: Duration,
) -> Result<()> {
    let mut last_tick = Instant::now();

    loop {
        terminal.draw(|f| ui(f, &mut app))?;

        let timeout = tick_rate
            .checked_sub(last_tick.elapsed())
            .unwrap_or_else(|| Duration::from_secs(0));

        if crossterm::event::poll(timeout)? {
            if let Event::Key(key) = event::read()? {
                match key.code {
                    KeyCode::Char('q') => {
                        app.kill();
                        return Ok(());
                    }
                    KeyCode::Delete => app.kill(),
                    KeyCode::Char('w') => app.control(ControlDirection::Forward),
                    KeyCode::Char('d') => app.control(ControlDirection::Right),
                    KeyCode::Char('s') => app.control(ControlDirection::Backwards),
                    KeyCode::Char('a') => app.control(ControlDirection::Left),
                    KeyCode::Char(' ') => app.control(ControlDirection::Break),
                    KeyCode::Up => app.select(SelectOp::UP)?,
                    KeyCode::Down => app.select(SelectOp::DOWN)?,
                    KeyCode::Enter => app.select(SelectOp::MARK)?,
                    _ => (),
                }
            }
        }

        if last_tick.elapsed() >= tick_rate {
            app.on_tick()?;
            last_tick = Instant::now();
        }
    }
}

fn ui<B: Backend>(f: &mut Frame<B>, app: &mut App) {
    // Common layout
    let size = f.size();
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .margin(2)
        .constraints([Constraint::Min(2), Constraint::Length(3)].as_ref())
        .split(size);

    // Top layout
    let top = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(20), Constraint::Percentage(80)].as_ref())
        .split(chunks[0]);

    // Devices
    let items: Vec<ListItem> = app
        .nodes
        .iter()
        .map(|node| {
            ListItem::new(Spans::from(vec![Span::styled(
                {
                    let leader = if node.leader { " (leader)" } else { "" };

                    format!(
                        "{:X}: {} dBm{}",
                        node.mac,
                        node.rssi.last().map(|(_, r)| r).unwrap_or(&0),
                        leader
                    )
                },
                Style::default().fg(Color::Indexed(((node.mac % 6) + 9).try_into().unwrap())),
            )]))
        })
        .collect();

    let devices = List::new(items)
        .block(Block::default().title("Nodes").borders(Borders::ALL))
        .style(Style::default().fg(Color::White))
        .highlight_style(Style::default().add_modifier(Modifier::ITALIC))
        .highlight_symbol(">");
    f.render_stateful_widget(devices, top[0], &mut app.selected);

    // Chart
    let epoch_seconds = app.epoch.elapsed().as_secs_f64();

    let chart = Chart::new(app.get_datasets())
        .block(
            Block::default()
                .title(Span::styled(
                    "Signal Strength",
                    Style::default()
                        .fg(Color::Cyan)
                        .add_modifier(Modifier::BOLD),
                ))
                .borders(Borders::ALL),
        )
        .x_axis(
            Axis::default()
                .title("Time")
                .style(Style::default().fg(Color::Gray))
                .labels(vec![
                    Span::styled("-30 seconds", Style::default().add_modifier(Modifier::BOLD)),
                    Span::styled("Now", Style::default().add_modifier(Modifier::BOLD)),
                ])
                .bounds([epoch_seconds - 30., epoch_seconds]),
        )
        .y_axis(
            Axis::default()
                .title("RSSI")
                .style(Style::default().fg(Color::Gray))
                .labels(vec![
                    Span::styled("-99 dBm", Style::default().add_modifier(Modifier::BOLD)),
                    Span::raw("-50 dBm"),
                    Span::styled("-0 dBm", Style::default().add_modifier(Modifier::BOLD)),
                ])
                .bounds([-100., -0.]),
        );
    f.render_widget(chart, top[1]);

    // Help
    let help_text = "q: Quit | Del: Kill Switch | WASD: Control | Space: Brake | Up/Down: Select Node | Enter: Mark Leader".to_string();
    let help = Paragraph::new(help_text)
        .style(Style::default().fg(Color::LightCyan))
        .alignment(Alignment::Center)
        .block(
            Block::default()
                .borders(Borders::ALL)
                .style(Style::default().fg(Color::White))
                .title("Help")
                .border_type(BorderType::Plain),
        );
    f.render_widget(help, chunks[1]);
}
