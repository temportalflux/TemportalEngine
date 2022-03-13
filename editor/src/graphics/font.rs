use crate::{
	asset::{BuildPath, TypeEditorMetadata},
	engine::{
		asset::{AnyBox, AssetResult},
		graphics::font::Font,
	},
};
use anyhow::Result;
use engine::task::PinFutureResultLifetime;
use serde_json;
use std::{
	path::{Path, PathBuf},
	time::SystemTime,
};

#[path = "sdf-builder.rs"]
mod sdf_builder;
pub use sdf_builder::*;

pub static LOG: &'static str = "font-builder";

pub struct EditorMetadata {}

impl EditorMetadata {
	fn font_path(&self, asset_path: &Path) -> PathBuf {
		let mut path = asset_path.parent().unwrap().to_path_buf();
		path.push(asset_path.file_stem().unwrap());
		path.set_extension("ttf");
		path
	}
}

impl TypeEditorMetadata for EditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata + 'static + Send + Sync> {
		Box::new(Self {})
	}

	fn last_modified(&self, path: &Path) -> Result<SystemTime> {
		let mut max_last_modified_at = path.metadata()?.modified()?;
		let ttf_path = self.font_path(&path);
		if ttf_path.exists() {
			let last_modified_at = path.metadata()?.modified()?;
			max_last_modified_at = max_last_modified_at.max(last_modified_at);
		}
		Ok(max_last_modified_at)
	}

	fn read(&self, _path: &Path, json_str: &str) -> AssetResult {
		let font: Font = serde_json::from_str(json_str)?;
		Ok(Box::new(font))
	}

	fn compile<'a>(
		&'a self,
		build_path: &'a BuildPath,
		asset: AnyBox,
	) -> PinFutureResultLifetime<'a, Vec<u8>> {
		Box::pin(async move {
			use freetype::Library;
			let mut font = asset.downcast::<Font>().unwrap();

			// TODO: only initialize this once per build
			let font_library = Library::init()?;

			let sdf = SDFBuilder::default()
				.with_font_path(&self.font_path(&build_path.source))
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

			let bytes = rmp_serde::to_vec(&font)?;
			Ok(bytes)
		})
	}
}
