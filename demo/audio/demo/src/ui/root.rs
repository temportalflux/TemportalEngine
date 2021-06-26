use super::*;

#[pre_hooks(use_button_notified_state)]
pub fn widget(mut ctx: WidgetContext) -> WidgetNode {
	WidgetNode::Component(
		make_widget!(nav_vertical_box)
			.listed_slot(make_widget!(track_control::widget).key("audio/music-for-manatees")),
	)
}
