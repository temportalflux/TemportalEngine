use crate::engine::ui::raui::*;

pub fn widget(mut _context: WidgetContext) -> WidgetNode {
	WidgetNode::Component(
		make_widget!(nav_vertical_box)
			.listed_slot(make_widget!(super::log::widget))
			.listed_slot(
				make_widget!(super::input::widget).with_props(FlexBoxItemLayout {
					grow: 0.0,
					shrink: 0.0,
					basis: Some(30.0),
					..Default::default()
				}),
			),
	)
}
