use super::*;

fn use_root(context: &mut WidgetContext) {
	context.life_cycle.change(|context| {
		for msg in context.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if msg.trigger_start() {
					match msg.sender.key() {
						"play-music" => {
							log::debug!("play the music");
						}
						"pause-music" => {
							log::debug!("pause the music");
						}
						_ => {}
					}
				}
			}
		}
	});
}

#[pre_hooks(use_root, use_button_notified_state)]
pub fn widget(mut ctx: WidgetContext) -> WidgetNode {
	WidgetNode::Component(
		make_widget!(nav_vertical_box).listed_slot(
			make_widget!(nav_horizontal_box)
				.listed_slot(
					make_widget!(size_box)
						.with_props(SizeBoxProps {
							width: SizeBoxSizeValue::Exact(20.0),
							height: SizeBoxSizeValue::Exact(20.0),
							..Default::default()
						})
						.named_slot(
							"content",
							make_widget!(image_button::widget)
								.key("play-music")
								.with_props(image_button::Props {
									image_id: "ui/play".to_owned(),
								})
								.with_props(ButtonNotifyProps(ctx.id.to_owned().into()))
								.with_props(FlexBoxItemLayout {
									grow: 0.0,
									..Default::default()
								}),
						),
				)
				.listed_slot(
					make_widget!(image_button::widget)
						.key("pause-music")
						.with_props(image_button::Props {
							image_id: "ui/pause".to_owned(),
						})
						.with_props(ButtonNotifyProps(ctx.id.to_owned().into()))
						.with_props(FlexBoxItemLayout {
							grow: 0.0,
							..Default::default()
						}),
				),
		),
	)
}
