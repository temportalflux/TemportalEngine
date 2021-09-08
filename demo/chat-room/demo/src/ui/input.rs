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
						// get the user's input without the newline
						let mut user_input = text.clone();
						user_input.pop();
						log::debug!("{}", user_input);

						// clear the field
						let mut input_state =
							context.state.read_cloned_or_default::<TextInputProps>();
						input_state.text = "".to_string();
						let _ = context.state.write(input_state);
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
		if cursor_position < text.len() {
			format!("{}|{}", &text[..cursor_position], &text[cursor_position..])
		} else {
			format!("{}|", text)
		}
	} else {
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
