use crate::Editor;
use egui::{TextureHandle, Vec2};
use engine::{
	asset::{self, Loader},
	graphics::Texture,
	ui::egui::Ui,
	Application,
};
use enumset::{EnumSet, EnumSetType};
use std::{
	collections::HashMap,
	mem::MaybeUninit,
	sync::{Arc, Once, RwLock},
};

fn loaded_icons() -> &'static Arc<RwLock<HashMap<Icon, TextureHandle>>> {
	static mut DATA: (MaybeUninit<Arc<RwLock<HashMap<Icon, TextureHandle>>>>, Once) =
		(MaybeUninit::uninit(), Once::new());
	unsafe {
		DATA.1.call_once(|| {
			DATA.0
				.as_mut_ptr()
				.write(Arc::new(RwLock::new(HashMap::new())))
		});
		&*DATA.0.as_ptr()
	}
}

#[derive(EnumSetType, Debug, Hash)]
pub enum Icon {
	Folder,
}

impl Icon {
	fn asset_id(&self) -> asset::Id {
		match self {
			Self::Folder => Editor::get_asset_id("icons/open-folder"),
		}
	}

	pub fn load_all(ui: Arc<RwLock<Ui>>) {
		use engine::task::{global_scopes, join_all, scope::*};
		let task_scope = Scope::named("Load UI Icons", "load-ui-icons");
		let unattached = task_scope.clone().spawn_silently(async move {
			let icons = EnumSet::<Self>::all();
			let mut subscopes = Vec::with_capacity(icons.len());
			for icon in icons.into_iter() {
				let scope = Scope::named(format!("Load {icon:?}"), format!("load-icon({icon:?})"));
				let unattached = scope.spawn(Self::load(icon, ui.clone()));
				let attached = unattached.attach_to(&task_scope);
				subscopes.push(attached);
			}
			join_all(subscopes).await?;
			Ok(())
		});
		unattached.attach_to(global_scopes());
	}

	async fn load(self, ui: Arc<RwLock<Ui>>) -> anyhow::Result<()> {
		let asset_id = self.asset_id();
		let texture = Loader::load(&asset_id)
			.await?
			.downcast::<Texture>()
			.unwrap();
		let image_data = {
			let color_data = egui::ColorImage::from_rgba_unmultiplied(
				[texture.size().x, texture.size().y],
				&texture.binary()[..],
			);
			egui::ImageData::Color(color_data)
		};
		let handle = {
			let mut ui = ui.write().unwrap();
			ui.context_mut()
				.load_texture(asset_id.as_string(), image_data)
		};
		if let Ok(mut loaded) = loaded_icons().write() {
			loaded.insert(self, handle);
		}
		Ok(())
	}

	pub fn show(self, ui: &mut egui::Ui, size: Option<Vec2>) -> Option<egui::Response> {
		let handle = match loaded_icons().read() {
			Ok(icons) => match icons.get(&self) {
				Some(handle) => handle.clone(),
				None => return None,
			},
			Err(_) => return None,
		};
		Some(ui.image(&handle, size.unwrap_or(handle.size_vec2())))
	}
}
