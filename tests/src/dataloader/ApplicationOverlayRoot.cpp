#include "openvic-simulation/GameManager.hpp"

#include <filesystem>
#include <fstream>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Application data root overrides base lookup before mods",
	"[dataloader][application-overlay]"
) {
	namespace fs = std::filesystem;

	const fs::path temp =
		fs::temp_directory_path() / "openvic-live-economy-004-overlay";
	const fs::path base = temp / "base";
	const fs::path overlay = temp / "overlay";

	fs::remove_all(temp);
	fs::create_directories(base / "common");
	fs::create_directories(overlay / "common");

	{
		std::ofstream out { base / "common" / "probe.txt" };
		out << "base";
	}
	{
		std::ofstream out { overlay / "common" / "probe.txt" };
		out << "overlay";
	}

	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	const std::array<fs::path, 1> base_roots { base };
	REQUIRE(manager.set_base_path(base_roots));
	REQUIRE(manager.add_application_data_root(overlay));

	const fs::path resolved =
		manager.get_dataloader().lookup_file("common/probe.txt");

	CHECK(resolved == overlay / "common" / "probe.txt");

	fs::remove_all(temp);
}