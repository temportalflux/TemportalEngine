use crate::{math::Vector, utility, window, Application, Engine};
use temportal_graphics::{device::physical, AppInfo};

pub struct Builder {
	title: String,
	inner_logical_size: Vector<f64, 2>,
	resizable: bool,
	app_info: AppInfo,
	constraints: Vec<physical::Constraint>,
	render_pass_clear_color: Vector<f32, 4>,
}

impl Default for Builder {
	fn default() -> Builder {
		Builder {
			title: String::new(),
			inner_logical_size: Vector::default(),
			resizable: false,
			app_info: AppInfo::default(),
			constraints: physical::default_constraints(),
			render_pass_clear_color: Vector::new([0.0, 0.0, 0.0, 1.0]),
		}
	}
}

impl Builder {
	pub fn with_title(mut self, title: &str) -> Self {
		self.title = title.to_string();
		self
	}

	pub fn with_size(mut self, width: f64, height: f64) -> Self {
		self.inner_logical_size = Vector::new([width, height]);
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

	pub fn with_clear_color(mut self, color: Vector<f32, 4>) -> Self {
		self.render_pass_clear_color = color;
		self
	}

	pub fn build(self, engine: &Engine) -> Result<window::Window, utility::AnyError> {
		optick::event!();
		log::info!(
			target: window::LOG,
			"Creating window \"{}\" with size {}",
			self.title,
			self.inner_logical_size
		);
		Ok(window::Window::new(
			winit::window::WindowBuilder::new()
				.with_title(self.title)
				.with_inner_size(winit::dpi::Size::Logical(winit::dpi::LogicalSize::new(
					self.inner_logical_size.x(),
					self.inner_logical_size.y(),
				)))
				.with_resizable(self.resizable)
				.build(engine.event_loop())?,
			self.app_info,
			self.constraints,
			self.render_pass_clear_color,
		)?)
	}
}
