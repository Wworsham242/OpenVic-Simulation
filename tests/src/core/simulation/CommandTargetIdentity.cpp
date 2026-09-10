#include "openvic-simulation/core/simulation/CommandTargetIdentity.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Command target identity requires target and jurisdiction",
	"[convergence][authority][target][validation]"
) {
	CHECK_FALSE(CommandTargetIdentity {}.is_canonical());

	CHECK_FALSE(CommandTargetIdentity {
		.target_id = "target:A",
		.jurisdiction_id = ""
	}.is_canonical());

	CHECK_FALSE(CommandTargetIdentity {
		.target_id = "",
		.jurisdiction_id = "jurisdiction:A"
	}.is_canonical());

	CHECK(CommandTargetIdentity {
		.target_id = "target:A",
		.jurisdiction_id = "jurisdiction:A"
	}.is_canonical());
}

TEST_CASE(
	"Resolved command target must match requested jurisdiction",
	"[convergence][authority][target][jurisdiction]"
) {
	CommandTargetIdentity const target {
		.target_id = "target:A",
		.jurisdiction_id = "jurisdiction:A"
	};

	CHECK(target.matches_jurisdiction("jurisdiction:A"));
	CHECK_FALSE(target.matches_jurisdiction("jurisdiction:B"));
	CHECK_FALSE(target.matches_jurisdiction(""));
}
