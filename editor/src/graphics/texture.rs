use engine::task::PinFutureResult;

use crate::{
	asset::{BuildPath, EditorOps},
	engine::{asset::AnyBox, graphics::Texture, math::nalgebra},
};
use std::path::{Path, PathBuf};

pub struct TextureEditorOps;

impl TextureEditorOps {
	fn image_file_path(json_path: &Path) -> PathBuf {
		let mut png_path = json_path.parent().unwrap().to_path_buf();
		png_path.push(json_path.file_stem().unwrap());
		png_path.set_extension("png");
		png_path
	}
}

impl EditorOps for TextureEditorOps {
	type Asset = Texture;

	fn get_related_paths(path: PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> {
		Box::pin(async move { Ok(Some(vec![Self::image_file_path(&path)])) })
	}

	fn read(source: PathBuf, file_content: String) -> PinFutureResult<AnyBox> {
		Box::pin(async move { crate::asset::deserialize::<Texture>(&source, &file_content) })
	}

	fn compile(build_path: BuildPath, asset: AnyBox) -> PinFutureResult<Vec<u8>> {
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

			let bytes = rmp_serde::to_vec_named(&texture)?;
			Ok(bytes)
		})
	}
}
