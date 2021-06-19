use crate::engine::ui::*;

#[pre_hooks(use_button_notified_state, use_text_input_notified_state)]
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
	log::debug!("{}", text);
	WidgetNode::Component(
		make_widget!(input_field)
			.key(key)
			.with_props(NavItemActive)
			.with_props(TextInputNotifyProps(id.to_owned().into()))
			.with_props(ButtonNotifyProps(id.to_owned().into()))
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
