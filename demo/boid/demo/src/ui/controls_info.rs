use crate::{
	ecs::systems::{CreateEntityMessage, DestroyEntityMessage},
	engine::{
		asset::statics,
		ui::{self, raui::*},
	},
	ui::{carousel, counter, CounterNotify, CounterProps},
	Archetype, GameContext,
};
use serde::{Deserialize, Serialize};

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize, PartialEq, Eq)]
struct State {
	batch_count: usize,
	type_index: usize,
}

impl State {
	fn get(ctx_state: &ui::raui::State) -> Self {
		ctx_state.read_cloned_or_default()
	}
}

fn use_controls_info(ctx: &mut WidgetContext) {
	ctx.life_cycle.change(move |ctx| {
		let mut state = State::get(&ctx.state);
		for msg in ctx.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if msg.trigger_start() {
					match msg.sender.key() {
						"create" => GameContext::write().enqueue_create(CreateEntityMessage {
							count: state.batch_count,
							archetype: Archetype::all()[state.type_index],
						}),
						"destroy" => GameContext::write().enqueue_destroy(DestroyEntityMessage {
							count: state.batch_count,
						}),
						_ => {}
					}
				}
			} else if let Some(msg) = msg.as_any().downcast_ref::<CounterNotify>() {
				state = State {
					batch_count: msg.state.value as usize,
					..state
				};
			} else if let Some(msg) = msg.as_any().downcast_ref::<carousel::NotifyMessage>() {
				state = State {
					type_index: msg.value,
					..state
				};
			}
		}
		let _ = ctx.state.write(state);
	});
}

#[pre_hooks(use_controls_info)]
pub fn controls_info(mut context: WidgetContext) -> WidgetNode {
	let WidgetContext { id, props, .. } = context;
	WidgetNode::Component(
		make_widget!(nav_vertical_box)
			.merge_props(props.clone())
			.listed_slot(
				make_widget!(text_box)
					.key("header")
					.with_props(TextBoxProps {
						text: "Controls".to_owned(),
						font: TextBoxFont {
							name: statics::font::unispace::BOLD.to_owned(),
							size: 40.0,
						},
						..Default::default()
					})
					.with_props(FlexBoxItemLayout {
						grow: 0.0, // text should not fill its container's size
						// temporary work around until text widgets can have their size calculated https://github.com/RAUI-labs/raui/issues/32
						basis: Some(40.0),
						..Default::default()
					}),
			)
			.listed_slot(
				make_widget!(size_box)
					.with_props(SizeBoxProps {
						width: SizeBoxSizeValue::Exact(100.0),
						height: SizeBoxSizeValue::Exact(50.0),
						..Default::default()
					})
					.named_slot(
						"content",
						make_widget!(counter)
							.key("batch-count")
							.with_props(CounterProps {
								range: 1..11,
								default: 10,
							})
							.with_props(ButtonNotifyProps(id.to_owned().into())),
					)
					.with_props(FlexBoxItemLayout {
						grow: 0.0,
						basis: Some(30.0),
						..Default::default()
					}),
			)
			.listed_slot(
				make_widget!(carousel::widget)
					.key("boid_type_selector")
					.with_props(carousel::NotifyProps(id.to_owned().into()))
					.with_props(carousel::Props {
						default: 0,
						range: 0..Archetype::all().len(),
					})
					.named_slot(
						"content",
						make_widget!(carousel::text_content)
							.with_props(TextBoxProps {
								font: statics::font::unispace::REGULAR.at_size(30.0),
								..Default::default()
							})
							.with_props(carousel::TextContentProps {
								options: Archetype::all()
									.iter()
									.map(|archetype| archetype.display_name().to_owned())
									.collect(),
							}),
					),
			)
			.listed_slot(
				make_widget!(horizontal_box)
					.listed_slot(
						make_widget!(button)
							.key("create")
							.with_props(NavItemActive)
							.with_props(ButtonNotifyProps(id.to_owned().into()))
							.named_slot(
								"content",
								make_widget!(text_box)
									.key("label")
									.with_props(TextBoxProps {
										text: "Spawn".to_owned(),
										font: statics::font::unispace::REGULAR.at_size(30.0),
										..Default::default()
									}),
							),
					)
					.listed_slot(
						make_widget!(button)
							.key("destroy")
							.with_props(NavItemActive)
							.with_props(ButtonNotifyProps(id.to_owned().into()))
							.named_slot(
								"content",
								make_widget!(text_box)
									.key("label")
									.with_props(TextBoxProps {
										text: "Kill".to_owned(),
										font: statics::font::unispace::REGULAR.at_size(30.0),
										..Default::default()
									}),
							),
					)
					.with_props(FlexBoxItemLayout {
						grow: 0.0,
						basis: Some(30.0),
						..Default::default()
					}),
			)
			.listed_slot(make_widget!(space_box)),
	)
}
