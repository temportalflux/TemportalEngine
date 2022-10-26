use crate::{
	asset::{BuildPath, EditorOps},
	engine::{asset::AnyBox, graphics::font::Font},
};
use engine::task::PinFutureResult;
use serde_json;
use std::path::{Path, PathBuf};

#[path = "sdf-builder.rs"]
mod sdf_builder;
pub use sdf_builder::*;

pub static LOG: &'static str = "font-builder";

pub struct FontEditorOps {}

impl FontEditorOps {
	fn font_path(asset_path: &Path) -> PathBuf {
		let mut path = asset_path.parent().unwrap().to_path_buf();
		path.push(asset_path.file_stem().unwrap());
		path.set_extension("ttf");
		path
	}
}

impl EditorOps for FontEditorOps {
	type Asset = Font;

	fn get_related_paths(path: PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> {
		Box::pin(async move { Ok(Some(vec![Self::font_path(&path)])) })
	}

	fn read(_source: PathBuf, file_content: String) -> PinFutureResult<AnyBox> {
		Box::pin(async move {
			let font: Font = serde_json::from_str(&file_content)?;
			let boxed: AnyBox = Box::new(font);
			Ok(boxed)
		})
	}

	fn compile(build_path: BuildPath, asset: AnyBox) -> PinFutureResult<Vec<u8>> {
		Box::pin(async move {
			use freetype::Library;
			let mut font = asset.downcast::<Font>().unwrap();

			// TODO: only initialize this once per build
			let font_library = Library::init()?;

			let sdf = SDFBuilder::default()
				.with_font_path(&Self::font_path(&build_path.source))
				.with_glyph_height(50)
				.with_spread(10)
				.with_padding([8; 4].into())
				.with_minimum_atlas_size([1024, 512].into())
				.build(&font_library)?;

			let png_path = build_path.source_with_ext("png");

			let mut img = image::RgbaImage::new(sdf.size.x as u32, sdf.size.y as u32);
			for pos_x in 0..sdf.size.x {
				for pos_y in 0..sdf.size.y {
					img.put_pixel(
						pos_x as u32,
						pos_y as u32,
						image::Rgba([255, 255, 255, sdf.binary[pos_y][pos_x]]),
					);
				}
			}
			img.save_with_format(png_path, image::ImageFormat::Png)?;

			font.set_sdf(sdf);

			let bytes = rmp_serde::to_vec_named(&font)?;
			Ok(bytes)
		})
	}
}
