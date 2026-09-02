#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"

#include <cstdint>
#include <limits>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("SimulationTimeline advances monotonically and rejects invalid movement", "[foundation][timeline]") {
	SimulationTimeline timeline;

	CHECK(timeline.current_time().ticks() == 0);
	CHECK(timeline.advance(24));
	CHECK(timeline.current_time().ticks() == 24);
	CHECK(timeline.advance(48));
	CHECK(timeline.current_time().ticks() == 72);

	CHECK_FALSE(timeline.advance(-1));
	CHECK(timeline.current_time().ticks() == 72);
}

TEST_CASE("SimulationTimeline owns deterministic scheduled wake-ups", "[foundation][timeline][events]") {
	SimulationTimeline timeline;

	REQUIRE(timeline.schedule_event(
		SimTime::from_ticks(24),
		SimulationEventPayload { .type_id = "first", .data = { 1 } }
	).has_value());
	REQUIRE(timeline.schedule_event(
		SimTime::from_ticks(24),
		SimulationEventPayload { .type_id = "second", .data = { 2 } }
	).has_value());

	CHECK(timeline.scheduled_event_count() == 2);
	CHECK_FALSE(timeline.pop_due_event().has_value());

	REQUIRE(timeline.advance(24));

	const auto first = timeline.pop_due_event();
	const auto second = timeline.pop_due_event();
	REQUIRE(first.has_value());
	REQUIRE(second.has_value());

	CHECK(first->payload.type_id == "first");
	CHECK(second->payload.type_id == "second");
	CHECK(first->sequence < second->sequence);
	CHECK(timeline.scheduled_event_count() == 0);
}

TEST_CASE("SimulationTimeline restore preserves future wake-up behavior", "[foundation][timeline][restore]") {
	SimulationTimeline original;
	REQUIRE(original.advance(48));
	REQUIRE(original.schedule_event(
		SimTime::from_ticks(72),
		SimulationEventPayload { .type_id = "future", .data = { 7 } }
	).has_value());

	const SimTime saved_time = original.current_time();
	const uint64_t next_sequence = original.next_event_sequence();
	const auto saved_events = original.snapshot_events();

	SimulationTimeline restored;
	REQUIRE(restored.restore(saved_time, next_sequence, saved_events));

	CHECK(restored.current_time() == saved_time);
	CHECK(restored.scheduled_event_count() == 1);
	CHECK_FALSE(restored.pop_due_event().has_value());

	REQUIRE(restored.advance(24));
	const auto delivered = restored.pop_due_event();
	REQUIRE(delivered.has_value());
	CHECK(delivered->payload.type_id == "future");
	CHECK(delivered->payload.data == std::vector<uint8_t> { 7 });
}

TEST_CASE("SimulationTimeline refuses forward overflow without partial mutation", "[foundation][timeline]") {
	SimulationTimeline timeline;
	REQUIRE(timeline.restore(
		SimTime::from_ticks(std::numeric_limits<int64_t>::max() - 1),
		0,
		{}
	));

	CHECK_FALSE(timeline.advance(2));
	CHECK(timeline.current_time().ticks() == std::numeric_limits<int64_t>::max() - 1);
}