use crate::engine::ui::raui::*;
use crate::ui::*;

pub fn root(mut _context: WidgetContext) -> WidgetNode {
	WidgetNode::Component(
		// The root widget is a content box (i.e. overlay)
		make_widget!(content_box).listed_slot(
			// Which has a size box that contains the `controls_info` display
			make_widget!(size_box)
				// The size box is anchored to the bottom left corner of the screen
				.with_props(ContentBoxItemLayout {
					anchors: Rect {
						left: 0.0,
						right: 0.0,
						top: 1.0,
						bottom: 0.0,
					},
					align: Vec2 { x: 0.0, y: 1.0 },
					..Default::default()
				})
				// And has a fixed size
				.with_props(SizeBoxProps {
					width: SizeBoxSizeValue::Exact(250.0),
					height: SizeBoxSizeValue::Exact(150.0),
					..Default::default()
				})
				.named_slot(
					"content",
					// The content of the size box has an overlay/content_box with 2 elements
					make_widget!(content_box)
						// The first is the background of the area (which is just a translucent black box)
						.listed_slot(make_widget!(image_box).with_props(ImageBoxProps {
							material: ImageBoxMaterial::Color(ImageBoxColor {
								color: Color {
									r: 0.0,
									g: 0.0,
									b: 0.0,
									a: 0.6,
								},
								..Default::default()
							}),
							..Default::default()
						}))
						// The second is the actual controls informational display,
						// which is clipped to the area of the size box (because the content box takes up the full size box).
						.listed_slot(make_widget!(controls_info).key("controls")),
				),
		), /*
		   .listed_slot(
			   make_widget!(text_box)
				   .with_props(ContentBoxItemLayout {
					   anchors: Rect {
						   left: 0.2,
						   right: 0.0,
						   top: 0.4,
						   bottom: 0.0,
					   },
					   align: Vec2 { x: 0.5, y: 0.5 },
					   ..Default::default()
				   })
				   .with_props(TextBoxProps {
					   text: "Starting Soon".to_owned(),
					   font: TextBoxFont {
						   name: crate::engine::asset::statics::font::unispace::REGULAR.to_owned(),
						   size: 150.0,
					   },
					   ..Default::default()
				   }),
		   ),
		   */
	)
}
