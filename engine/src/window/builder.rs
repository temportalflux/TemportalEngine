use crate::{
	graphics::{device::physical, AppInfo},
	math::nalgebra::{Vector2, Vector4},
	utility, window, Application, Engine,
};

pub struct Builder {
	title: String,
	inner_logical_size: Vector2<f64>,
	resizable: bool,
	app_info: AppInfo,
	constraints: Vec<physical::Constraint>,
	render_pass_clear_color: Vector4<f32>,
}

impl Default for Builder {
	fn default() -> Builder {
		Builder {
			title: String::new(),
			inner_logical_size: [0.0, 0.0].into(),
			resizable: false,
			app_info: AppInfo::default(),
			constraints: physical::default_constraints(),
			render_pass_clear_color: [0.0, 0.0, 0.0, 1.0].into(),
		}
	}
}

impl Builder {
	pub fn with_title(mut self, title: &str) -> Self {
		self.title = title.to_string();
		self
	}

	pub fn with_size(mut self, width: f64, height: f64) -> Self {
		self.inner_logical_size = [width, height].into();
		self
	}

	pub fn with_resizable(mut self, resizable: bool) -> Self {
		self.resizable = resizable;
		self
	}

	pub fn with_application<T: Application>(mut self) -> Self {
		self.app_info = crate::make_app_info::<T>();
		self
	}

	pub fn with_constraints(mut self, constraints: Vec<physical::Constraint>) -> Self {
		self.constraints = constraints;
		self
	}

	pub fn with_clear_color(mut self, color: Vector4<f32>) -> Self {
		self.render_pass_clear_color = color;
		self
	}

	#[profiling::function]
	pub fn build(self, engine: &mut Engine) -> Result<&mut window::Window, utility::AnyError> {
		log::info!(
			target: window::LOG,
			"Creating window \"{}\" with size <{},{}>",
			self.title,
			self.inner_logical_size.x,
			self.inner_logical_size.y,
		);
		let mut window = window::Window::new(
			winit::window::WindowBuilder::new()
				.with_title(self.title)
				.with_inner_size(winit::dpi::Size::Logical(winit::dpi::LogicalSize::new(
					self.inner_logical_size.x,
					self.inner_logical_size.y,
				)))
				.with_position(winit::dpi::Position::Logical(
					winit::dpi::LogicalPosition::new(0.0, 0.0),
				))
				.with_resizable(self.resizable)
				.build(engine.event_loop())?,
			self.app_info,
			self.constraints,
			self.render_pass_clear_color,
		)?;
		window.create_render_chain()?;

		{
			use crate::input::{self, event};
			let (physical_size, scale_factor) = window.read_size();
			input::send_event(event::Event::Window(
				event::WindowEvent::ScaleFactorChanged(
					physical_size.width,
					physical_size.height,
					scale_factor,
				),
			));
		}

		Ok(engine.set_window(window))
	}
}
