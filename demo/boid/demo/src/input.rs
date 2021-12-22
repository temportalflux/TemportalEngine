pub use crate::engine::input::{self, *};

pub static ACTION_CREATE_BOID: &'static str = "CreateBoid";
pub static ACTION_DESTROY_BOID: &'static str = "DestroyBoid";
pub static ACTION_SELECT_PREV_BOID: &'static str = "SelectBoid-Prev";
pub static ACTION_SELECT_NEXT_BOID: &'static str = "SelectBoid-Next";
pub static ACTION_SELECT_NONE_BOID: &'static str = "SelectBoid-None";

pub fn init() {
	use crate::engine::input::{
		binding::{ActionMap, ActionSet, ActionSetId, Gamepad::*, LayoutId, Source::*},
		device::GamepadKind::*,
		source::{Button::*, Key::*, Kind},
	};
	input::write()
		.add_users(1)
		.add_action(ACTION_CREATE_BOID, Kind::Button)
		.add_action(ACTION_DESTROY_BOID, Kind::Button)
		.add_action(ACTION_SELECT_PREV_BOID, Kind::Button)
		.add_action(ACTION_SELECT_NEXT_BOID, Kind::Button)
		.add_action(ACTION_SELECT_NONE_BOID, Kind::Button)
		.add_layout(LayoutId::default())
		.add_action_set(
			ActionSetId::default(),
			ActionSet::default().with(
				LayoutId::default(),
				ActionMap::default()
					.bind(
						ACTION_CREATE_BOID,
						Keyboard(Equals) + Keyboard(NumpadPlus) + Gamepad(DualAxisGamepad, Button(VirtualConfirm))
					)
					.bind(
						ACTION_DESTROY_BOID,
						Keyboard(Minus) + Keyboard(NumpadMinus)
					)
					.bind(ACTION_SELECT_PREV_BOID, Keyboard(Comma))
					.bind(ACTION_SELECT_NEXT_BOID, Keyboard(Period))
					.bind(ACTION_SELECT_NONE_BOID, Keyboard(Slash)),
			),
		)
		.enable_action_set_for_all(ActionSetId::default());
}
