#include "openvic-simulation/core/simulation/LiveCommandTimelineSnapshot.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	void populate(
		SimulationTimeline& timeline,
		OrderedCommandRuntime& commands
	) {
		REQUIRE(timeline.advance(48));
		REQUIRE(timeline.schedule_event(
			SimTime::from_ticks(72),
			SimulationEventPayload { .type_id = "continuity.event", .data = { 9, 8, 7 } }
		).has_value());

		REQUIRE(commands.accept(
			SimTime::from_ticks(24),
			"office:executive:USA",
			"military.set_mobilised",
			"country:USA",
			{ 1, 0, 0, 0, 1 }
		).has_value());
		REQUIRE(commands.accept(
			SimTime::from_ticks(48),
			"institution:central_bank:USA",
			"monetary.set_target",
			"country:USA",
			{ 4, 2 }
		).has_value());
		REQUIRE(commands.consume_next_replay().has_value());
	}
}

TEST_CASE("Live command timeline capture is canonical", "[reconciliation][persistence][runtime]") {
	SimulationTimeline timeline;
	OrderedCommandRuntime commands;
	populate(timeline, commands);

	auto const snapshot = LiveCommandTimelineState::capture(timeline, commands);

	CHECK(snapshot.is_canonical());
	CHECK(snapshot.timeline.current_time == SimTime::from_ticks(48));
	CHECK(snapshot.replay.accepted_command_count == 2);
	CHECK(snapshot.replay.replay_cursor == 1);
	CHECK(snapshot.command_log.size() == 2);
}

TEST_CASE("Live command timeline restores exact continuity", "[reconciliation][persistence][replay]") {
	SimulationTimeline original_timeline;
	OrderedCommandRuntime original_commands;
	populate(original_timeline, original_commands);

	auto const snapshot = LiveCommandTimelineState::capture(original_timeline, original_commands);

	SimulationTimeline restored_timeline;
	OrderedCommandRuntime restored_commands;

	REQUIRE(LiveCommandTimelineState::restore(
		snapshot,
		restored_timeline,
		restored_commands
	));

	CHECK(restored_timeline.capture_snapshot() == original_timeline.capture_snapshot());
	CHECK(restored_commands.capture_replay_state() == original_commands.capture_replay_state());
	CHECK(restored_commands.capture_command_log() == original_commands.capture_command_log());
}

TEST_CASE("Live command timeline restore is transactional across both owners", "[reconciliation][persistence][transaction]") {
	SimulationTimeline timeline;
	OrderedCommandRuntime commands;
	populate(timeline, commands);

	auto const before_timeline = timeline.capture_snapshot();
	auto const before_replay = commands.capture_replay_state();
	auto const before_log = commands.capture_command_log();

	auto bad = LiveCommandTimelineState::capture(timeline, commands);
	bad.command_log[1].sequence = 99;

	CHECK_FALSE(LiveCommandTimelineState::restore(bad, timeline, commands));
	CHECK(timeline.capture_snapshot() == before_timeline);
	CHECK(commands.capture_replay_state() == before_replay);
	CHECK(commands.capture_command_log() == before_log);

	bad = LiveCommandTimelineState::capture(timeline, commands);
	bad.timeline.schema_version += 1;

	CHECK_FALSE(LiveCommandTimelineState::restore(bad, timeline, commands));
	CHECK(timeline.capture_snapshot() == before_timeline);
	CHECK(commands.capture_replay_state() == before_replay);
	CHECK(commands.capture_command_log() == before_log);
}