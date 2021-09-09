use crate::engine::ui::*;

fn use_message_input(context: &mut WidgetContext) {
	context.life_cycle.change(|context| {
		for msg in context.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref() {
				match msg {
					TextInputNotifyMessage {
						state: TextInputProps { text, .. },
						..
					} if text.ends_with("\n") => {
						use crate::engine::network::prelude::{
							DeliveryGuarantee::*, OrderGuarantee::*, *,
						};

						// get the user's input without the newline
						let mut user_input = text.clone();
						user_input.pop();
						log::debug!("{}", user_input);

						// clear the field
						let mut input_state =
							context.state.read_cloned_or_default::<TextInputProps>();
						input_state.text = "".to_string();
						// DEBUG NOTE: No error is ever logged
						if let Err(e) = context.state.write(input_state) {
							log::error!("Failed to write state: {:?}", e);
						}

						// DEBUG NOTE: Reread the props out of the state
						let input_state = context.state.read_cloned_or_default::<TextInputProps>();
						// DEBUG NOTE: The text outputted here is always the same as `user_input`, even though the state was written/cleared
						log::debug!("text cleared? \"{}\"", input_state.text);

						let _ = Network::send_to_server(
							Packet::builder()
								.with_guarantee(Reliable + Ordered)
								.with_payload(&crate::packet::Message::new(user_input)),
						);
					}
					_ => {}
				}
			}
		}
	});
}

#[pre_hooks(
	use_button_notified_state,
	use_text_input_notified_state,
	use_message_input
)]
pub fn widget(mut context: WidgetContext) -> WidgetNode {
	let WidgetContext { id, state, key, .. } = context;
	let ButtonProps {
		selected, trigger, ..
	} = state.read_cloned_or_default();
	let TextInputProps {
		text,
		cursor_position,
		focused,
		..
	} = state.read_cloned_or_default();
	let text = if text.trim().is_empty() {
		"> Focus here and start typing...".to_owned()
	} else if focused {
		log::debug!("text: {}", text);
		if cursor_position < text.len() {
			format!("{}|{}", &text[..cursor_position], &text[cursor_position..])
		} else {
			format!("{}|", text)
		}
	} else {
		log::debug!("text: {}", text);
		text
	};
	WidgetNode::Component(
		make_widget!(input_field)
			.key(key)
			.with_props(NavItemActive)
			.with_props(TextInputNotifyProps(id.to_owned().into()))
			.with_props(ButtonNotifyProps(id.to_owned().into()))
			// need to manually indicate that this is a text field until https://github.com/RAUI-labs/raui/pull/73 is fixed
			.with_props(TextInputMode::Text)
			.with_props(TextInputProps {
				allow_new_line: true,
				..Default::default()
			})
			.named_slot(
				"content",
				make_widget!(text_box).with_props(TextBoxProps {
					text,
					font: TextBoxFont {
						name: crate::engine::asset::statics::font::unispace::REGULAR.to_owned(),
						size: 20.0,
					},
					color: if trigger {
						Color {
							r: 1.0,
							g: 0.0,
							b: 0.0,
							a: 1.0,
						}
					} else if selected {
						Color {
							r: 0.0,
							g: 1.0,
							b: 0.0,
							a: 1.0,
						}
					} else if focused {
						Color {
							r: 0.0,
							g: 0.0,
							b: 1.0,
							a: 1.0,
						}
					} else {
						Color {
							r: 0.0,
							g: 0.0,
							b: 0.0,
							a: 1.0,
						}
					},
					..Default::default()
				}),
			),
	)
}
