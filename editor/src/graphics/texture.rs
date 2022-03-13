use crate::{
	asset::{BuildPath, TypeEditorMetadata},
	engine::{
		asset::{AnyBox, AssetResult},
		graphics::Texture,
		math::nalgebra,
	},
};
use anyhow::Result;
use engine::task::PinFutureResultLifetime;
use std::{
	path::{Path, PathBuf},
	time::SystemTime,
};

pub struct TextureEditorMetadata {}

impl TextureEditorMetadata {
	fn image_file_path(json_path: &Path) -> PathBuf {
		let mut png_path = json_path.parent().unwrap().to_path_buf();
		png_path.push(json_path.file_stem().unwrap());
		png_path.set_extension("png");
		png_path
	}
}

impl TypeEditorMetadata for TextureEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata + 'static + Send + Sync> {
		Box::new(TextureEditorMetadata {})
	}

	fn last_modified(&self, path: &Path) -> Result<SystemTime> {
		Ok(path
			.metadata()?
			.modified()?
			.max(Self::image_file_path(path).metadata()?.modified()?))
	}

	fn read(&self, path: &Path, content: &str) -> AssetResult {
		crate::asset::deserialize::<Texture>(&path, &content)
	}

	fn compile<'a>(
		&'a self,
		build_path: &'a BuildPath,
		asset: AnyBox,
	) -> PinFutureResultLifetime<'a, Vec<u8>> {
		Box::pin(async move {
			use image::Pixel;
			let mut texture = asset.downcast::<Texture>().unwrap();

			let rgba_image = image::open(Self::image_file_path(&build_path.source))?.into_rgba8();
			let size = nalgebra::vector![rgba_image.width() as usize, rgba_image.height() as usize];
			let mut binary = Vec::with_capacity(size.x * size.y * 4);
			for (_y, row) in rgba_image.enumerate_rows() {
				for (_x, _y, pixel) in row {
					for channel in pixel.channels() {
						binary.push(*channel);
					}
				}
			}
			texture.set_compiled(size, binary);

			let bytes = rmp_serde::to_vec(&texture)?;
			Ok(bytes)
		})
	}
}
