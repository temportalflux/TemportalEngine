use crate::engine::{asset::statics, ui::raui::*};
use serde::{Deserialize, Serialize};

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize)]
pub struct CounterProps {
	pub range: std::ops::Range<i32>,
	pub default: i32,
}

#[derive(PropsData, Debug, Default, Copy, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct CounterState {
	pub value: i32,
}

#[derive(MessageData, Debug, Clone)]
pub struct CounterNotify {
	pub state: CounterState,
}

fn update_state<F>(ctx: &WidgetMountOrChangeContext, callback: F)
where
	F: Fn(&mut CounterState),
{
	let prev = ctx.state.read_cloned_or_default::<CounterState>();
	let mut next = prev.clone();
	callback(&mut next);
	if next != prev {
		write_state(&ctx, next);
	}
}

fn write_state(ctx: &WidgetMountOrChangeContext, state: CounterState) {
	let _ = ctx.state.write(state);
	notify(&ctx, state);
}

fn notify(ctx: &WidgetMountOrChangeContext, state: CounterState) {
	if let Ok(ButtonNotifyProps(notify)) = ctx.props.read() {
		if let Some(to) = notify.read() {
			ctx.messenger.write(to, CounterNotify { state });
		}
	}
}

fn use_counter(ctx: &mut WidgetContext) {
	ctx.life_cycle.mount(|ctx| {
		write_state(
			&ctx,
			CounterState {
				value: ctx.props.read_cloned_or_default::<CounterProps>().default,
			},
		);
	});
	ctx.life_cycle.change(|ctx| {
		let props = ctx.props.read_cloned_or_default::<CounterProps>();
		for msg in ctx.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if msg.trigger_start() {
					match msg.sender.key() {
						"add" => {
							update_state(&ctx, |state| {
								state.value = (state.value + 1).min(props.range.end - 1);
							});
						}
						"sub" => {
							update_state(&ctx, |state| {
								state.value = (state.value - 1).max(props.range.start);
							});
						}
						_ => {}
					}
				}
			}
		}
	});
}

#[pre_hooks(use_counter)]
pub fn counter(mut ctx: WidgetContext) -> WidgetNode {
	let WidgetContext { id, state, .. } = ctx;

	let make_arrow = |scale_x: f32| {
		make_widget!(image_box).with_props(ImageBoxProps {
			width: ImageBoxSizeValue::Exact(10.0),
			height: ImageBoxSizeValue::Exact(20.0),
			material: ImageBoxMaterial::Image(ImageBoxImage {
				id: "arrow".to_owned(),
				..Default::default()
			}),
			transform: Transform {
				scale: Vec2 { x: scale_x, y: 1.0 },
				pivot: Vec2 { x: 0.5, y: 0.5 },
				..Default::default()
			},
			..Default::default()
		})
	};

	let make_arrow_button = |key: &str, scale_x: f32| {
		make_widget!(button)
			.key(key)
			.with_props(NavItemActive)
			.with_props(ButtonNotifyProps(id.to_owned().into()))
			.named_slot("content", make_arrow(scale_x))
	};

	fn wrap(margin: Rect, content: WidgetComponent) -> WidgetComponent {
		make_widget!(wrap_box)
			.with_props(WrapBoxProps {
				margin,
				..Default::default()
			})
			.named_slot("content", content)
			.with_props(FlexBoxItemLayout {
				grow: 0.0,
				..Default::default()
			})
	}

	let text = format!("{}", state.read_cloned_or_default::<CounterState>().value);

	WidgetNode::Component(
		make_widget!(nav_horizontal_box)
			.listed_slot(wrap(10.0.into(), make_arrow_button("sub", -1.0)))
			.listed_slot(
				make_widget!(text_box)
					.key("label")
					.with_props(TextBoxProps {
						text,
						font: statics::font::unispace::REGULAR.at_size(30.0),
						..Default::default()
					}),
			)
			.listed_slot(wrap(10.0.into(), make_arrow_button("add", 1.0))),
	)
}
