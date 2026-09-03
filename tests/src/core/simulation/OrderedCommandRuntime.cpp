#include "openvic-simulation/core/simulation/OrderedCommandRuntime.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	OrderedCommandRuntime make_runtime() {
		OrderedCommandRuntime runtime;
		REQUIRE(runtime.accept(
			SimTime::from_ticks(24),
			"actor:executive:USA",
			"policy.set_priority",
			{ 1, 2, 3 }
		).has_value());
		REQUIRE(runtime.accept(
			SimTime::from_ticks(24),
			"actor:central_bank:USA",
			"monetary.set_target",
			{ 4, 5 }
		).has_value());
		REQUIRE(runtime.accept(
			SimTime::from_ticks(48),
			"actor:military_command:USA",
			"force.set_readiness",
			{ 6 }
		).has_value());
		return runtime;
	}
}

TEST_CASE("Accepted commands receive stable contiguous sequence", "[foundation][command][ordering]") {
	OrderedCommandRuntime runtime;

	auto a = runtime.accept(SimTime::from_ticks(5), "actor:a", "command.one", { 1 });
	auto b = runtime.accept(SimTime::from_ticks(5), "actor:b", "command.two", { 2 });
	auto c = runtime.accept(SimTime::from_ticks(1), "actor:c", "command.three", { 3 });

	REQUIRE(a.has_value());
	REQUIRE(b.has_value());
	REQUIRE(c.has_value());
	CHECK(*a == 0);
	CHECK(*b == 1);
	CHECK(*c == 2);

	// Acceptance order is authoritative even if submitted_at values are non-monotonic.
	const auto log = runtime.capture_command_log();
	REQUIRE(log.size() == 3);
	CHECK(log[0].actor_id == "actor:a");
	CHECK(log[1].actor_id == "actor:b");
	CHECK(log[2].actor_id == "actor:c");
}

TEST_CASE("Human AI institution and military identities share same envelope", "[foundation][command][actors]") {
	OrderedCommandRuntime runtime;

	REQUIRE(runtime.accept(SimTime::from_ticks(1), "human:office:PM", "policy.propose", {}).has_value());
	REQUIRE(runtime.accept(SimTime::from_ticks(1), "ai:office:PM", "policy.propose", {}).has_value());
	REQUIRE(runtime.accept(SimTime::from_ticks(1), "institution:central_bank", "rate.review", {}).has_value());
	REQUIRE(runtime.accept(SimTime::from_ticks(1), "military:theater_command", "plan.activate", {}).has_value());

	CHECK(runtime.accepted_command_count() == 4);
}

TEST_CASE("Replay consumes exact accepted order", "[foundation][command][replay]") {
	OrderedCommandRuntime runtime = make_runtime();

	for (uint64_t expected = 0; expected < 3; ++expected) {
		auto command = runtime.consume_next_replay();
		REQUIRE(command.has_value());
		CHECK(command->sequence == expected);
	}
	CHECK_FALSE(runtime.consume_next_replay().has_value());
	CHECK(runtime.replay_cursor() == 3);

	runtime.reset_replay_cursor();
	REQUIRE(runtime.peek_next_replay().has_value());
	CHECK(runtime.peek_next_replay()->sequence == 0);
	CHECK(runtime.replay_cursor() == 0);
}

TEST_CASE("Command runtime snapshot restore continues replay identically", "[foundation][command][persistence][replay]") {
	OrderedCommandRuntime original = make_runtime();

	REQUIRE(original.consume_next_replay().has_value());

	const CampaignReplayState replay = original.capture_replay_state();
	const auto log = original.capture_command_log();

	OrderedCommandRuntime restored;
	REQUIRE(restored.restore(replay, log));

	CHECK(restored.capture_replay_state() == replay);
	CHECK(restored.capture_command_log() == log);

	while (true) {
		auto a = original.consume_next_replay();
		auto b = restored.consume_next_replay();
		CHECK(a.has_value() == b.has_value());
		if (!a.has_value()) {
			break;
		}
		CHECK(*a == *b);
	}
}

TEST_CASE("Invalid command restore is transactional", "[foundation][command][validation]") {
	OrderedCommandRuntime runtime = make_runtime();
	const auto before_log = runtime.capture_command_log();
	const auto before_replay = runtime.capture_replay_state();

	auto bad_log = before_log;
	bad_log[1].sequence = 99;
	CHECK_FALSE(runtime.restore(before_replay, bad_log));
	CHECK(runtime.capture_command_log() == before_log);
	CHECK(runtime.capture_replay_state() == before_replay);

	bad_log = before_log;
	bad_log[0].actor_id.clear();
	CHECK_FALSE(runtime.restore(before_replay, bad_log));
	CHECK(runtime.capture_command_log() == before_log);

	auto bad_replay = before_replay;
	bad_replay.replay_cursor = bad_replay.accepted_command_count + 1;
	CHECK_FALSE(runtime.restore(bad_replay, before_log));
	CHECK(runtime.capture_replay_state() == before_replay);
}

TEST_CASE("Command state composes canonically into campaign snapshot", "[foundation][command][campaign]") {
	OrderedCommandRuntime runtime = make_runtime();

	CampaignStateSnapshot campaign;
	campaign.command_log = runtime.capture_command_log();
	campaign.replay = runtime.capture_replay_state();

	CHECK(campaign.is_canonical());

	const uint64_t baseline = campaign.checksum();
	campaign.command_log[0].payload.push_back(99);
	CHECK(campaign.checksum() != baseline);
}

TEST_CASE("Command acceptance rejects missing actor or command type", "[foundation][command][validation]") {
	OrderedCommandRuntime runtime;
	CHECK_FALSE(runtime.accept(SimTime::from_ticks(0), "", "command", {}).has_value());
	CHECK_FALSE(runtime.accept(SimTime::from_ticks(0), "actor", "", {}).has_value());
	CHECK(runtime.accepted_command_count() == 0);
}