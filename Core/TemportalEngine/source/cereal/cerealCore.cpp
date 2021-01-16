#include "cereal/cerealCore.hpp"

cereal::JSONOutputArchive::Options cereal::JsonFormat = cereal::JSONOutputArchive::Options(
	324,
	cereal::JSONOutputArchive::Options::IndentChar::tab, 1
);
