use super::*;
use serde::{Deserialize, Serialize};

fn use_button(ctx: &mut WidgetContext) {
	ctx.life_cycle.change(|ctx| {
		for msg in ctx.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if let Ok(ButtonNotifyProps(notify)) = ctx.props.read() {
					if let Some(to) = notify.read() {
						ctx.messenger.write(
							to,
							ButtonNotifyMessage {
								sender: ctx.id.clone(),
								..msg.clone()
							},
						);
					}
				}
			}
		}
	});
}

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize)]
pub struct Props {
	pub image_id: String,
}

// use_button_notified_state - exposes `ButtonProps` as a valid state for this widget, which contains the stateful behavior data
#[pre_hooks(use_button, use_button_notified_state)]
pub fn widget(mut ctx: WidgetContext) -> WidgetNode {
	let ButtonProps {
		// if the button is hovered via mouse or navigated to view gamepad/keyboard
		selected: is_hovered,
		// if the button is currently pressed/active because of a click or key/gamepad button input
		trigger: is_active,
		..
	} = ctx.state.read_cloned_or_default();
	let Props { image_id } = ctx.props.read_cloned_or_default();
	WidgetNode::Component(
		make_widget!(button)
			.key("button")
			.with_props(NavItemActive)
			.with_props(ButtonNotifyProps(ctx.id.to_owned().into()))
			.named_slot(
				"content",
				make_widget!(image_box).with_props(ImageBoxProps {
					width: ImageBoxSizeValue::Exact(50.0),
					height: ImageBoxSizeValue::Exact(50.0),
					material: ImageBoxMaterial::Image(ImageBoxImage {
						id: image_id,
						tint: {
							if is_active {
								// https://colorpicker.me/#ede179
								Color {
									r: 0.93,
									g: 0.88,
									b: 0.47,
									a: 1.0,
								}
							} else if is_hovered {
								// https://colorpicker.me/#d7cf8a
								Color {
									r: 0.84,
									g: 0.81,
									b: 0.54,
									a: 1.0,
								}
							} else {
								Color {
									r: 1.0,
									g: 1.0,
									b: 1.0,
									a: 1.0,
								}
							}
						},
						..Default::default()
					}),
					..Default::default()
				}),
			),
	)
}
