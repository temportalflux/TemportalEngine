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
