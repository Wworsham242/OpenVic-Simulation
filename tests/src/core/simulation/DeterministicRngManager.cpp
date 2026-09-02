#include "openvic-simulation/core/simulation/DeterministicRngManager.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("Named RNG stream is repeatable from same master seed", "[foundation][rng][determinism]") {
	DeterministicRngManager a { 123456789 };
	DeterministicRngManager b { 123456789 };

	for (int i = 0; i < 100; ++i) {
		CHECK(a.next_u64("combat.operational") == b.next_u64("combat.operational"));
	}

	REQUIRE(a.draw_count("combat.operational").has_value());
	CHECK(*a.draw_count("combat.operational") == 100);
}

TEST_CASE("Named RNG streams are isolated from each other", "[foundation][rng][streams]") {
	DeterministicRngManager baseline { 42 };
	DeterministicRngManager perturbed { 42 };

	std::vector<uint64_t> expected;
	for (int i = 0; i < 16; ++i) {
		expected.push_back(baseline.next_u64("economy.market"));
	}

	for (int i = 0; i < 500; ++i) {
		(void)perturbed.next_u64("ai.strategic");
	}

	for (uint64_t expected_value : expected) {
		CHECK(perturbed.next_u64("economy.market") == expected_value);
	}
}

TEST_CASE("Different stream ids produce distinct sequences", "[foundation][rng][streams]") {
	DeterministicRngManager rng { 99 };

	bool any_difference = false;
	for (int i = 0; i < 16; ++i) {
		if (rng.next_u64("combat.air") != rng.next_u64("combat.naval")) {
			any_difference = true;
		}
	}
	CHECK(any_difference);
}

TEST_CASE("RNG snapshot restore continues exact future sequence", "[foundation][rng][persistence][replay]") {
	DeterministicRngManager original { 8675309 };

	for (int i = 0; i < 37; ++i) {
		(void)original.next_u64("combat.operational");
	}
	for (int i = 0; i < 11; ++i) {
		(void)original.next_u64("politics.domestic");
	}

	const auto state = original.capture_state();

	DeterministicRngManager restored;
	REQUIRE(restored.restore_state(original.master_seed(), state));
	CHECK(restored.capture_state() == state);

	for (int i = 0; i < 1000; ++i) {
		CHECK(
			restored.next_u64("combat.operational")
			== original.next_u64("combat.operational")
		);
	}
	for (int i = 0; i < 100; ++i) {
		CHECK(
			restored.next_u64("politics.domestic")
			== original.next_u64("politics.domestic")
		);
	}

	CHECK(restored.capture_state() == original.capture_state());
}

TEST_CASE("RNG capture order is canonical by stream id", "[foundation][rng][persistence]") {
	DeterministicRngManager rng { 1 };

	(void)rng.next_u64("zeta");
	(void)rng.next_u64("alpha");
	(void)rng.next_u64("middle");

	const auto state = rng.capture_state();
	REQUIRE(state.size() == 3);
	CHECK(state[0].stream_id == "alpha");
	CHECK(state[1].stream_id == "middle");
	CHECK(state[2].stream_id == "zeta");
}

TEST_CASE("Invalid RNG restore is transactional", "[foundation][rng][persistence][validation]") {
	DeterministicRngManager rng { 5 };
	(void)rng.next_u64("alpha");
	const auto before = rng.capture_state();
	const uint64_t before_seed = rng.master_seed();

	auto bad = before;
	bad[0].stream_id.clear();
	CHECK_FALSE(rng.restore_state(999, bad));
	CHECK(rng.master_seed() == before_seed);
	CHECK(rng.capture_state() == before);

	bad = {
		CampaignRngStreamState { .stream_id = "b", .state_lo = 1, .state_hi = 2, .draw_count = 0 },
		CampaignRngStreamState { .stream_id = "a", .state_lo = 3, .state_hi = 4, .draw_count = 0 }
	};
	CHECK_FALSE(rng.restore_state(999, bad));
	CHECK(rng.master_seed() == before_seed);
	CHECK(rng.capture_state() == before);

	bad = {
		CampaignRngStreamState { .stream_id = "zero", .state_lo = 0, .state_hi = 0, .draw_count = 0 }
	};
	CHECK_FALSE(rng.restore_state(999, bad));
	CHECK(rng.master_seed() == before_seed);
	CHECK(rng.capture_state() == before);
}

TEST_CASE("RNG campaign records remain canonical for campaign envelope", "[foundation][rng][campaign]") {
	DeterministicRngManager rng { 77 };
	(void)rng.next_u64("combat.operational");
	(void)rng.next_u64("ai.strategic");

	CampaignStateSnapshot campaign;
	campaign.rng_streams = rng.capture_state();

	// Empty ECS identity is valid at composition level; timeline defaults canonical.
	CHECK(campaign.is_canonical());
}