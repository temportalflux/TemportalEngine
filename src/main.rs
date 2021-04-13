
fn main() -> Result<(), Box<dyn std::error::Error>> {
	temportal_engine::run(std::env::args().collect())
}
