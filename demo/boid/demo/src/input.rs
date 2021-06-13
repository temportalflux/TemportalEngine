pub use crate::engine::input::{self, *};

pub static ACTION_CREATE_BOID: &'static str = "CreateBoid";
pub static ACTION_DESTROY_BOID: &'static str = "DestroyBoid";
pub static ACTION_SELECT_PREV_BOID: &'static str = "SelectBoid-Prev";
pub static ACTION_SELECT_NEXT_BOID: &'static str = "SelectBoid-Next";
pub static ACTION_SELECT_NONE_BOID: &'static str = "SelectBoid-None";

pub fn init() {
	use crate::engine::input::{
		action::Action,
		binding::{ActionMap, ActionSet, ActionSetId, Gamepad::*, LayoutId, Source::*},
		device::GamepadKind::*,
		source::{Button::*, Key::*, Kind},
	};
	input::write()
		.add_users(1)
		.add_action(ACTION_CREATE_BOID, Action::new(Kind::Button))
		.add_action(ACTION_DESTROY_BOID, Action::new(Kind::Button))
		.add_action(ACTION_SELECT_PREV_BOID, Action::new(Kind::Button))
		.add_action(ACTION_SELECT_NEXT_BOID, Action::new(Kind::Button))
		.add_action(ACTION_SELECT_NONE_BOID, Action::new(Kind::Button))
		.add_layout(LayoutId::default())
		.add_action_set(
			ActionSetId::default(),
			ActionSet::default().with(
				LayoutId::default(),
				ActionMap::default()
					.bind(
						ACTION_CREATE_BOID,
						vec![
							Keyboard(Equals).bound(),
							Keyboard(NumpadPlus).bound(),
							Gamepad(DualAxisGamepad, Button(VirtualConfirm)).bound(),
						],
					)
					.bind(
						ACTION_DESTROY_BOID,
						vec![Keyboard(Minus).bound(), Keyboard(NumpadMinus).bound()],
					)
					.bind(ACTION_SELECT_PREV_BOID, vec![Keyboard(Comma).bound()])
					.bind(ACTION_SELECT_NEXT_BOID, vec![Keyboard(Period).bound()])
					.bind(ACTION_SELECT_NONE_BOID, vec![Keyboard(Slash).bound()]),
			),
		)
		.enable_action_set_for_all(ActionSetId::default());
}
