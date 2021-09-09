use crate::engine::ui::*;
use crate::MessageHistory;

pub fn widget(mut _context: WidgetContext) -> WidgetNode {
	let mut log_box = make_widget!(vertical_box).with_props(FlexBoxItemLayout {
		grow: 1.0,
		..Default::default()
	});

	if let Ok(history) = MessageHistory::read() {
		for message in history.iter_messages() {
			let widget = make_widget!(text_box)
				.with_props(TextBoxProps {
					text: message.content.clone(),
					font: TextBoxFont {
						name: crate::engine::asset::statics::font::unispace::REGULAR.to_owned(),
						size: 20.0,
					},
					..Default::default()
				})
				.with_props(FlexBoxItemLayout {
					grow: 0.0,
					basis: Some(20.0),
					..Default::default()
				});
			log_box = log_box.listed_slot(widget);
		}
	}

	WidgetNode::Component(log_box)
}
