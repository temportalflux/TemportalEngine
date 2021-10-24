use egui::Key;
use winit::event::VirtualKeyCode;

pub fn key_code(key: VirtualKeyCode) -> Option<egui::Key> {
	Some(match key {
		VirtualKeyCode::Down => Key::ArrowDown,
		VirtualKeyCode::Left => Key::ArrowLeft,
		VirtualKeyCode::Right => Key::ArrowRight,
		VirtualKeyCode::Up => Key::ArrowUp,
		VirtualKeyCode::Escape => Key::Escape,
		VirtualKeyCode::Tab => Key::Tab,
		VirtualKeyCode::Back => Key::Backspace,
		VirtualKeyCode::Return => Key::Enter,
		VirtualKeyCode::Space => Key::Space,
		VirtualKeyCode::Insert => Key::Insert,
		VirtualKeyCode::Delete => Key::Delete,
		VirtualKeyCode::Home => Key::Home,
		VirtualKeyCode::End => Key::End,
		VirtualKeyCode::PageUp => Key::PageUp,
		VirtualKeyCode::PageDown => Key::PageDown,
		VirtualKeyCode::Key0 => Key::Num0,
		VirtualKeyCode::Key1 => Key::Num1,
		VirtualKeyCode::Key2 => Key::Num2,
		VirtualKeyCode::Key3 => Key::Num3,
		VirtualKeyCode::Key4 => Key::Num4,
		VirtualKeyCode::Key5 => Key::Num5,
		VirtualKeyCode::Key6 => Key::Num6,
		VirtualKeyCode::Key7 => Key::Num7,
		VirtualKeyCode::Key8 => Key::Num8,
		VirtualKeyCode::Key9 => Key::Num9,
		VirtualKeyCode::A => Key::A,
		VirtualKeyCode::B => Key::B,
		VirtualKeyCode::C => Key::C,
		VirtualKeyCode::D => Key::D,
		VirtualKeyCode::E => Key::E,
		VirtualKeyCode::F => Key::F,
		VirtualKeyCode::G => Key::G,
		VirtualKeyCode::H => Key::H,
		VirtualKeyCode::I => Key::I,
		VirtualKeyCode::J => Key::J,
		VirtualKeyCode::K => Key::K,
		VirtualKeyCode::L => Key::L,
		VirtualKeyCode::M => Key::M,
		VirtualKeyCode::N => Key::N,
		VirtualKeyCode::O => Key::O,
		VirtualKeyCode::P => Key::P,
		VirtualKeyCode::Q => Key::Q,
		VirtualKeyCode::R => Key::R,
		VirtualKeyCode::S => Key::S,
		VirtualKeyCode::T => Key::T,
		VirtualKeyCode::U => Key::U,
		VirtualKeyCode::V => Key::V,
		VirtualKeyCode::W => Key::W,
		VirtualKeyCode::X => Key::X,
		VirtualKeyCode::Y => Key::Y,
		VirtualKeyCode::Z => Key::Z,
		_ => return None,
	})
}

pub fn modifiers(modifiers: winit::event::ModifiersState) -> egui::Modifiers {
	egui::Modifiers {
		alt: modifiers.alt(),
		ctrl: modifiers.ctrl(),
		shift: modifiers.shift(),
		#[cfg(target_os = "macos")]
		mac_cmd: modifiers.logo(),
		#[cfg(target_os = "macos")]
		command: modifiers.logo(),
		#[cfg(not(target_os = "macos"))]
		mac_cmd: false,
		#[cfg(not(target_os = "macos"))]
		command: modifiers.ctrl(),
	}
}

pub fn mouse_button(button: winit::event::MouseButton) -> Option<egui::PointerButton> {
	Some(match button {
		winit::event::MouseButton::Left => egui::PointerButton::Primary,
		winit::event::MouseButton::Right => egui::PointerButton::Secondary,
		winit::event::MouseButton::Middle => egui::PointerButton::Middle,
		_ => return None,
	})
}

/// Convert from [`egui::CursorIcon`] to [`winit::window::CursorIcon`].
pub fn cursor_icon(cursor_icon: egui::CursorIcon) -> Option<winit::window::CursorIcon> {
	Some(match cursor_icon {
		egui::CursorIcon::Default => winit::window::CursorIcon::Default,
		egui::CursorIcon::PointingHand => winit::window::CursorIcon::Hand,
		egui::CursorIcon::ResizeHorizontal => winit::window::CursorIcon::ColResize,
		egui::CursorIcon::ResizeNeSw => winit::window::CursorIcon::NeResize,
		egui::CursorIcon::ResizeNwSe => winit::window::CursorIcon::NwResize,
		egui::CursorIcon::ResizeVertical => winit::window::CursorIcon::RowResize,
		egui::CursorIcon::Text => winit::window::CursorIcon::Text,
		egui::CursorIcon::Grab => winit::window::CursorIcon::Grab,
		egui::CursorIcon::Grabbing => winit::window::CursorIcon::Grabbing,
		egui::CursorIcon::None => return None,
		egui::CursorIcon::ContextMenu => winit::window::CursorIcon::ContextMenu,
		egui::CursorIcon::Help => winit::window::CursorIcon::Help,
		egui::CursorIcon::Progress => winit::window::CursorIcon::Progress,
		egui::CursorIcon::Wait => winit::window::CursorIcon::Wait,
		egui::CursorIcon::Cell => winit::window::CursorIcon::Cell,
		egui::CursorIcon::Crosshair => winit::window::CursorIcon::Crosshair,
		egui::CursorIcon::VerticalText => winit::window::CursorIcon::VerticalText,
		egui::CursorIcon::Alias => winit::window::CursorIcon::Alias,
		egui::CursorIcon::Copy => winit::window::CursorIcon::Copy,
		egui::CursorIcon::Move => winit::window::CursorIcon::Move,
		egui::CursorIcon::NoDrop => winit::window::CursorIcon::NoDrop,
		egui::CursorIcon::NotAllowed => winit::window::CursorIcon::NotAllowed,
		egui::CursorIcon::AllScroll => winit::window::CursorIcon::AllScroll,
		egui::CursorIcon::ZoomIn => winit::window::CursorIcon::ZoomIn,
		egui::CursorIcon::ZoomOut => winit::window::CursorIcon::ZoomOut,
	})
}
