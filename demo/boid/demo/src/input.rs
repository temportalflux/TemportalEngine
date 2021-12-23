use crate::engine::input::*;

pub static ACTION_CREATE_BOID: &'static str = "CreateBoid";
pub static ACTION_DESTROY_BOID: &'static str = "DestroyBoid";
pub static ACTION_SELECT_PREV_BOID: &'static str = "SelectBoid-Prev";
pub static ACTION_SELECT_NEXT_BOID: &'static str = "SelectBoid-Next";
pub static ACTION_SELECT_NONE_BOID: &'static str = "SelectBoid-None";

pub fn init() -> ArcLockUser {
	use prelude::{Source::Keyboard, *};
	engine::input::set_config(
		Config::default()
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
							Keyboard(Equals)
								+ Keyboard(NumpadPlus) + Source::Gamepad(
								DualAxisGamepad,
								Gamepad::Button(VirtualConfirm),
							),
						)
						.bind(ACTION_DESTROY_BOID, Keyboard(Minus) + Keyboard(NumpadMinus))
						.bind(ACTION_SELECT_PREV_BOID, Keyboard(Comma))
						.bind(ACTION_SELECT_NEXT_BOID, Keyboard(Period))
						.bind(ACTION_SELECT_NONE_BOID, Keyboard(Slash)),
				),
			),
	);
	let arc_user = engine::input::create_user("Local");
	if let Ok(mut user) = arc_user.write() {
		user.enable_action_set(ActionSetId::default());
	}
	arc_user
}
