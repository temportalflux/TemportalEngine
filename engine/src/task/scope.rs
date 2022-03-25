use std::sync::{Arc, Mutex, RwLock, Weak};
use tokio::task::{JoinError, JoinHandle};

pub trait Parent {
	fn attach<T>(self: &Arc<Self>, handle: Unattached<T>) -> JoinHandle<T>;
}

/// A collection of scopes which can be externally accessed.
/// This structure is used internally to keep track of all of the scopes a
/// given parent observers and should be used externally to hold weak reference to all
/// of the root-scopes which need to be [`sent_to`](Unattached::send_to) a scope list.
pub struct List(RwLock<Vec<Weak<Scope>>>, String);
impl List {
	pub fn new(name: String) -> Self {
		Self(RwLock::new(Vec::new()), name)
	}

	pub fn len(&self) -> usize {
		self.0.read().unwrap().len()
	}

	pub fn push(&self, scope: Weak<Scope>) {
		let mut children = self.0.write().unwrap();
		children.push(scope);
	}

	pub fn prune_expired_children(&self) {
		let mut children = self.0.write().unwrap();
		children.retain(|weak| weak.strong_count() > 0);
	}
}

impl Parent for List {
	fn attach<T>(self: &Arc<Self>, handle: Unattached<T>) -> JoinHandle<T> {
		if let Some(scope) = handle.0.upgrade() {
			if let Ok(mut parent) = scope.attached_to.lock() {
				*parent = Some(Arc::downgrade(&self));
			}
		}
		self.push(handle.0);
		handle.1
	}
}

/// A task scope provides the ability for users to report what async tasks were spawned and their lifetimes.
/// This is useful for debugging tools like looking at what user-spawned tasks are active at any given point of an application.
pub struct Scope {
	display_name: String,
	log_target: String,
	children: Arc<List>,
	attached_to: Mutex<Option<Weak<List>>>,
}

impl Scope {
	/// Create a new scope using the same name for both display name and log target.
	pub fn new<T: ToString>(name: T) -> Arc<Self> {
		Arc::new(Self {
			display_name: name.to_string(),
			log_target: name.to_string(),
			children: Arc::new(List::new(name.to_string())),
			attached_to: Mutex::new(None),
		})
	}

	/// Create a new scope using a set of names.
	pub fn named<T: ToString>(name: T, target: T) -> Arc<Self> {
		Arc::new(Self {
			display_name: name.to_string(),
			log_target: target.to_string(),
			children: Arc::new(List::new(target.to_string())),
			attached_to: Mutex::new(None),
		})
	}

	pub fn target(&self) -> &String {
		&self.log_target
	}

	/// Spawns a detacted future for a given scope. An [`Unattached`] scope is returned which provides
	/// a way for futures to await on/join with detached scopes.
	/// Equivalent to [`tokio::task::spawn`], but with additional instrumentation and error handling.
	pub fn spawn_silently<T, S>(self: Arc<Self>, future: T) -> Unattached<()>
	where
		T: futures::future::Future<Output = anyhow::Result<S>> + Send + 'static,
		S: Send + 'static,
	{
		self.spawn_mapped(future, |_s| {})
	}

	pub fn spawn<T, S>(self: Arc<Self>, future: T) -> Unattached<anyhow::Result<S>>
	where
		T: futures::future::Future<Output = anyhow::Result<S>> + Send + 'static,
		S: Send + 'static,
	{
		self.spawn_mapped(future, |s| s)
	}

	pub fn spawn_mapped<T, S, F, R>(self: Arc<Self>, future: T, map: F) -> Unattached<R>
	where
		T: futures::future::Future<Output = anyhow::Result<S>> + Send + 'static,
		F: Fn(anyhow::Result<S>) -> R + Send + 'static,
		S: Send + 'static,
		R: Send + 'static,
	{
		let weak_scope = Arc::downgrade(&self);
		let join_handle = tokio::task::spawn(async move {
			// this reference to self forces the async to take ownership of self
			let log_target = self.target();

			// Wait for spawned future to complete, and trace any error.
			let result = future.await;
			if let Err(err) = &result {
				log::error!(target: log_target, "{:?}", err);
			}

			// Scope/self is dropped when this scope completes, thereby marking any weak-refs as invalid.
			map(result)
		});
		Unattached(weak_scope, join_handle)
	}
}

impl Parent for Scope {
	/// Attaches a handle created by [`spawn`](Scope::spawn) such that the handle becomes a child of this scope.
	/// Returns an [`Attached`] indicating that handle has been attached but not awaited on.
	fn attach<T>(self: &Arc<Self>, handle: Unattached<T>) -> JoinHandle<T> {
		self.children.attach(handle)
	}
}

impl Scope {
	/// Takes some handle created by [`spawn`](Scope::spawn) and awaits on the detached future to complete.
	/// This is a short-hand for using `scope.attach(handle).await?`.
	pub async fn join<T>(self: &Arc<Self>, handle: Unattached<T>) -> Result<T, JoinError> {
		let attached = self.attach(handle);
		// attached will be dropped once await is complete, causing self to prune its children.
		let out = attached.await?;
		Ok(out)
	}

	/// Equivalent to [`join`] but takes a list of scopes to join on.
	/// Has the same drawbacks to [`join`] in that handles are not attached to the scope until this is called.
	pub async fn join_all(self: &Arc<Self>, handles: Vec<Unattached<()>>) -> anyhow::Result<()> {
		let handles_to_join = {
			let mut handles_to_join = Vec::with_capacity(handles.len());
			for Unattached(weak, handle) in handles.into_iter() {
				self.children.push(weak);
				handles_to_join.push(handle);
			}
			handles_to_join
		};
		let join_results = futures::future::join_all(handles_to_join).await;
		for result in join_results.into_iter() {
			if let Err(err) = result {
				return Err(err)?;
			}
		}
		self.children.prune_expired_children();
		Ok(())
	}
}

impl Drop for Scope {
	fn drop(&mut self) {
		// Ensure that the scope this attachment represented is removed
		// from the parent once the join_handle has been awaited on.
		if let Ok(parent) = self.attached_to.lock() {
			if let Some(scope_list) = &*parent {
				scope_list.upgrade().unwrap().prune_expired_children();
			}
		}
	}
}

/// The handle for some detached task which was spawned by [`Scope::spawn`].
///
/// Must either be sent to the global tracker or attached to a parent scope.
#[must_use = "Unattached scopes must either be sent to the global tracker or attached to some parent."]
pub struct Unattached<T>(Weak<Scope>, JoinHandle<T>);
impl<T> Unattached<T> {
	/// Alias for [`parent.attach(handle)`](Parent::attach).
	pub fn attach_to<TParent>(self, parent: &Arc<TParent>) -> JoinHandle<T>
	where
		TParent: Parent,
	{
		// This is the crux of the scope interface. Without calling this function
		// the root task of some task tree is never tracked and therefore cannot be reported on.
		parent.attach(self)
	}
}

// mock function
fn _init() {
	let scope1 = Scope::named("Scope 1", "scope1");
	let scope1 = scope1.clone().spawn_silently(async move {
		// Scope2 is created and then immediately attached and awaited on, blocking scope1 until scope2 is complete.
		// Functionally, this is no different than performing the async content of scope2 inline, but is shown like this to
		// highlight how a single scope can be joined to its parent.
		let scope2 = Scope::new("scope2").spawn_silently(async move { Ok(()) });
		// Scope2 will not be attached to/a child of scope1 until join is called. See Scope3 example on an alternative approach.
		scope1.join(scope2).await?;

		// Scope3 is an example of a delayed-await approach where the scope
		// is attached to the parent up-front and awaited on some time later.
		// Immediately attach to scope1 via attach_to.
		let scope3 = Scope::new("scope3")
			.spawn_silently(async move { Ok(()) })
			.attach_to(&scope1);
		// scope1 does some more processing...
		// and some n ms later, await on scope3
		scope3.await?;

		// Scope4 and Scope5 are run in parallel, and awaited on together before proceeding with Scope1
		let scope4 = Scope::new("scope4").spawn_silently(async move { Ok(()) });
		let scope5 = Scope::new("scope5").spawn_silently(async move { Ok(()) });
		scope1.join_all(vec![scope4, scope5]).await?;

		// Scope6 and Scope7 are run in parallel, but attached to Scope1 right away, and awaited on later.
		let scope6 = Scope::new("scope6").spawn_silently(async move { Ok(()) });
		let scope7 = Scope::new("scope7").spawn_silently(async move { Ok(()) });
		let scope6 = scope1.attach(scope6);
		let scope7 = scope1.attach(scope7);
		// scope1 does some more processing...
		// and some n ms later, await on scope6 & scope7
		let _ = futures::future::join_all(vec![scope6, scope7]).await;

		Ok(())
	});
	// In practice this scopes list would be some external item, but for simplicity a scope list is created here.
	let scopes = Arc::new(List::new("global".to_owned()));
	let _ = scope1.attach_to(&scopes);
}
