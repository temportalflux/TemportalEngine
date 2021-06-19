use crate::engine::ui::*;

pub fn widget(mut _context: WidgetContext) -> WidgetNode {
	WidgetNode::Component(
		make_widget!(vertical_box)
			.with_props(FlexBoxItemLayout {
				grow: 1.0,
				..Default::default()
			})
			.listed_slot(
				make_widget!(text_box)
					.with_props(TextBoxProps {
						text: "name: this is some line of text someone has sent".to_owned(),
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
					}),
			)
			.listed_slot(
				make_widget!(text_box)
					.with_props(TextBoxProps {
						text: "name2: and this is some response!".to_owned(),
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
					}),
			),
	)
}
