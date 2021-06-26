use super::*;
use crate::engine::Application;

fn use_track_control(context: &mut WidgetContext) {
	context.life_cycle.change(|context| {
		for msg in context.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if msg.trigger_start() {
					match msg.sender.key() {
						"play-music" => {
							if let Ok(mut audio_system) = crate::engine::audio::System::write() {
								if let Some(source) = audio_system
									.get_source(&crate::Demo::get_asset_id(context.id.key()))
								{
									use crate::engine::audio::source::Source;
									source.play(Some(1));
								} else {
									log::info!("{} has not finished loading yet", context.id.key());
								}
							}
						}
						"pause-music" => {
							if let Ok(mut audio_system) = crate::engine::audio::System::write() {
								if let Some(source) = audio_system
									.get_source(&crate::Demo::get_asset_id(context.id.key()))
								{
									use crate::engine::audio::source::Source;
									source.pause();
								} else {
									log::info!("{} has not finished loading yet", context.id.key());
								}
							}
						}
						_ => {}
					}
				}
			}
		}
	});
}

fn action_button(ctx: &WidgetContext, key: &str, image_id: &str) -> WidgetComponent {
	make_widget!(size_box)
		.with_props(SizeBoxProps {
			width: SizeBoxSizeValue::Exact(50.0),
			height: SizeBoxSizeValue::Exact(50.0),
			..Default::default()
		})
		.named_slot(
			"content",
			make_widget!(image_button::widget)
				.key(key)
				.with_props(image_button::Props {
					image_id: image_id.to_owned(),
				})
				.with_props(ButtonNotifyProps(ctx.id.to_owned().into())),
		)
		.with_props(FlexBoxItemLayout {
			grow: 0.0,
			..Default::default()
		})
}

#[pre_hooks(use_track_control)]
pub fn widget(mut ctx: WidgetContext) -> WidgetNode {
	WidgetNode::Component(
		make_widget!(nav_horizontal_box)
			.listed_slot(action_button(&ctx, "play-music", "ui/play"))
			.listed_slot(action_button(&ctx, "pause-music", "ui/pause"))
			.listed_slot(make_widget!(text_box).with_props(TextBoxProps {
				text: ctx.id.key().to_owned(),
				font: TextBoxFont {
					name: crate::engine::asset::statics::font::unispace::REGULAR.to_owned(),
					size: 40.0,
				},
				..Default::default()
			})),
	)
}
