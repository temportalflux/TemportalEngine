extern crate proc_macro;

use proc_macro::TokenStream;
use quote::quote;
use syn::{
	parse::{Parse, ParseStream, Result},
	parse_macro_input,
	ItemStruct, Token,
};

#[derive(Debug)]
struct PacketKindArgs {
	socknet_crate_path: syn::ExprPath,
	unique_id: syn::LitStr,
	processor_type: syn::ExprPath,
}

impl Parse for PacketKindArgs {
	fn parse(input: ParseStream) -> Result<Self> {
		let socknet_crate_path = input.parse()?;
		let _: Token![,] = input.parse()?;
		let unique_id = input.parse()?;
		let _: Token![,] = input.parse()?;
		let processor_type = input.parse()?;
		Ok(Self {
			socknet_crate_path,
			unique_id,
			processor_type,
		})
	}
}

#[proc_macro_attribute]
pub fn packet_kind(args: TokenStream, input: TokenStream) -> TokenStream {
	// tells rust that this macro must annotate a `struct`
	let item_struct = parse_macro_input!(input as ItemStruct);
	let name = &item_struct.ident;

	// ensure the `#[packet_kind]` macro has 2 specific arguments
	let PacketKindArgs {
		socknet_crate_path,
		unique_id,
		processor_type,
	} = parse_macro_input!(args as PacketKindArgs);

	// Construct the final metaprogramming,
	// implementing the `packet::Kind` and `Registerable<KindId, Registration>` traits for the struct.
	return quote! {
		#item_struct

		impl #socknet_crate_path::packet::Registerable<
			#socknet_crate_path::packet::KindId,
			#socknet_crate_path::packet::Registration
		> for #name {
			fn unique_id() -> #socknet_crate_path::packet::KindId {
				#unique_id
			}
			fn registration() -> #socknet_crate_path::packet::Registration
			where
				Self: Sized + 'static,
			{
				#socknet_crate_path::packet::Registration::of::<Self, #processor_type>()
			}
		}
		impl #socknet_crate_path::packet::Kind for #name {
			fn serialize_to(&self) -> Vec<u8> {
				#socknet_crate_path::serde::to_vec(&self).unwrap()
			}
			fn deserialize_from(bytes: &[u8]) -> AnyBox
			where
				Self: Sized,
			{
				Box::new(#socknet_crate_path::serde::from_read_ref::<[u8], #name>(&bytes).unwrap())
			}
		}
	}.into();
}
