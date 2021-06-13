use crate::engine::ui::{self, *};
use serde::{Deserialize, Serialize};

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize)]
pub struct Props {
	pub range: std::ops::Range<usize>,
	pub default: usize,
}

impl Props {
	fn get(ctx: &WidgetMountOrChangeContext) -> Self {
		ctx.props.read_cloned_or_default()
	}
}

#[derive(PropsData, Debug, Default, Copy, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct State {
	value: usize,
}

impl State {
	fn default(props: &Props) -> Self {
		Self {
			value: props.default,
		}
	}

	fn get(ctx_state: &ui::State) -> Self {
		ctx_state.read_cloned_or_default()
	}

	fn write(&self, ctx: &WidgetMountOrChangeContext) {
		let _ = ctx.state.write(self.clone());
		NotifyProps::write(ctx, NotifyMessage { value: self.value });
	}

	fn update<F>(ctx: &WidgetMountOrChangeContext, callback: F)
	where
		F: Fn(&mut Self),
	{
		let prev = Self::get(&ctx.state);
		let mut next = prev.clone();
		callback(&mut next);
		if next != prev {
			next.write(&ctx);
		}
	}
}

#[derive(MessageData, Debug, Clone)]
pub struct NotifyMessage {
	pub value: usize,
}

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize)]
pub struct NotifyProps(pub WidgetIdOrRef);

impl NotifyProps {
	fn write(ctx: &WidgetMountOrChangeContext, data: NotifyMessage) {
		if let Ok(Self(notify)) = ctx.props.read() {
			if let Some(to) = notify.read() {
				ctx.messenger.write(to, data);
			}
		}
	}
}

fn use_widget(ctx: &mut WidgetContext) {
	ctx.life_cycle.mount(|ctx| {
		State::default(&Props::get(&ctx)).write(&ctx);
	});
	ctx.life_cycle.change(|ctx| {
		let props = Props::get(&ctx);
		for msg in ctx.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if msg.trigger_start() {
					match msg.sender.key() {
						"add" => {
							State::update(&ctx, |state| {
								state.value = if state.value >= props.range.end - 1 {
									props.range.start
								} else {
									state.value + 1
								};
							});
						}
						"sub" => {
							State::update(&ctx, |state| {
								state.value = if state.value <= props.range.start {
									props.range.end - 1
								} else {
									state.value - 1
								};
							});
						}
						_ => {}
					}
				}
			}
		}
	});
}

#[pre_hooks(use_widget)]
pub fn widget(mut ctx: WidgetContext) -> WidgetNode {
	let WidgetContext {
		id,
		key,
		state,
		named_slots,
		..
	} = ctx;
	unpack_named_slots!(named_slots => { content });

	let make_arrow = |scale_x: f32| {
		make_widget!(image_box).with_props(ImageBoxProps {
			width: ImageBoxSizeValue::Exact(10.0),
			height: ImageBoxSizeValue::Exact(20.0),
			material: ImageBoxMaterial::Image(ImageBoxImage {
				id: "arrow".to_owned(),
				..Default::default()
			}),
			transform: Transform {
				scale: Vec2 { x: scale_x, y: 1.0 },
				pivot: Vec2 { x: 0.5, y: 0.5 },
				..Default::default()
			},
			..Default::default()
		})
	};

	let make_arrow_button = |key: &str, scale_x: f32| {
		make_widget!(button)
			.key(key)
			.with_props(NavItemActive)
			.with_props(ButtonNotifyProps(id.to_owned().into()))
			.named_slot("content", make_arrow(scale_x))
	};

	fn wrap(margin: Rect, content: WidgetComponent) -> WidgetComponent {
		make_widget!(wrap_box)
			.with_props(WrapBoxProps {
				margin,
				..Default::default()
			})
			.named_slot("content", content)
			.with_props(FlexBoxItemLayout {
				grow: 0.0,
				..Default::default()
			})
	}

	WidgetNode::Component(
		make_widget!(nav_horizontal_box)
			.key(key)
			.listed_slot(wrap(10.0.into(), make_arrow_button("sub", -1.0)))
			.listed_slot(match content {
				WidgetNode::Component(content) => {
					WidgetNode::Component(content.with_props(ContentProps {
						value: State::get(&state).value,
					}))
				}
				_ => content,
			})
			.listed_slot(wrap(10.0.into(), make_arrow_button("add", 1.0))),
	)
}

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize)]
pub struct ContentProps {
	pub value: usize,
}

#[derive(PropsData, Debug, Default, Clone, Serialize, Deserialize)]
pub struct TextContentProps {
	pub options: Vec<String>,
}

pub fn text_content(ctx: WidgetContext) -> WidgetNode {
	let content_props = ctx.props.read_cloned_or_default::<ContentProps>();
	let options = ctx
		.props
		.read_cloned_or_default::<TextContentProps>()
		.options;
	let text = options
		.get(content_props.value)
		.map(|v| v.clone())
		.unwrap_or("invalid".to_owned());
	let text_props = TextBoxProps {
		text,
		..ctx.props.read_cloned_or_default::<TextBoxProps>()
	};
	WidgetNode::Component(make_widget!(text_box).with_props(text_props))
}
