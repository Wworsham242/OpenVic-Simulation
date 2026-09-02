#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"

#include <cstdint>
#include <utility>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	void schedule_fixture(SimulationTimeline& timeline) {
		REQUIRE(timeline.schedule_event(
			SimTime::from_ticks(72),
			SimulationEventPayload { .type_id = "policy.review", .data = { 1, 2, 3 } }
		).has_value());
		REQUIRE(timeline.schedule_event(
			SimTime::from_ticks(72),
			SimulationEventPayload { .type_id = "shipment.arrival", .data = { 9, 8 } }
		).has_value());
		REQUIRE(timeline.schedule_event(
			SimTime::from_ticks(120),
			SimulationEventPayload { .type_id = "election.close", .data = { 4 } }
		).has_value());
	}

	int64_t drain_digest_until(SimulationTimeline& timeline, int64_t final_tick) {
		int64_t digest = 0;
		while (timeline.current_time().ticks() < final_tick) {
			REQUIRE(timeline.advance(1));
			while (auto event = timeline.pop_due_event()) {
				digest = digest * 1000003;
				digest += event->at.ticks() * 31;
				digest += static_cast<int64_t>(event->sequence) * 17;
				for (unsigned char ch : event->payload.type_id) {
					digest = digest * 131 + static_cast<int64_t>(ch);
				}
				for (uint8_t byte : event->payload.data) {
					digest = digest * 257 + static_cast<int64_t>(byte);
				}
			}
		}
		return digest;
	}
}

TEST_CASE("Timeline snapshot round-trip is exact", "[foundation][timeline][persistence]") {
	SimulationTimeline original;
	REQUIRE(original.advance(48));
	schedule_fixture(original);

	const SimulationTimelineSnapshot snapshot = original.capture_snapshot();

	SimulationTimeline restored;
	REQUIRE(restored.restore_snapshot(snapshot));

	CHECK(restored.capture_snapshot() == snapshot);
	CHECK(restored.capture_snapshot().checksum() == snapshot.checksum());
}

TEST_CASE("Timeline checksum covers durable state", "[foundation][timeline][persistence][checksum]") {
	SimulationTimeline timeline;
	schedule_fixture(timeline);
	const SimulationTimelineSnapshot baseline = timeline.capture_snapshot();
	const uint64_t baseline_checksum = baseline.checksum();

	auto changed = baseline;
	changed.current_time = SimTime::from_ticks(1);
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.next_event_sequence += 1;
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.events[0].at = SimTime::from_ticks(73);
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.events[0].payload.type_id += ".changed";
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.events[0].payload.data.push_back(99);
	CHECK(changed.checksum() != baseline_checksum);
}

TEST_CASE("Invalid timeline restore is transactional", "[foundation][timeline][persistence][validation]") {
	SimulationTimeline timeline;
	REQUIRE(timeline.advance(24));
	schedule_fixture(timeline);

	const SimulationTimelineSnapshot before = timeline.capture_snapshot();

	auto bad_version = before;
	bad_version.schema_version += 1;
	CHECK_FALSE(timeline.restore_snapshot(bad_version));
	CHECK(timeline.capture_snapshot() == before);

	auto bad_order = before;
	REQUIRE(bad_order.events.size() >= 2);
	std::swap(bad_order.events[0], bad_order.events[1]);
	CHECK_FALSE(timeline.restore_snapshot(bad_order));
	CHECK(timeline.capture_snapshot() == before);

	auto bad_cursor = before;
	bad_cursor.next_event_sequence = bad_cursor.events.back().sequence;
	CHECK_FALSE(timeline.restore_snapshot(bad_cursor));
	CHECK(timeline.capture_snapshot() == before);
}

TEST_CASE("Restored timeline produces identical deterministic future", "[foundation][timeline][persistence][replay]") {
	SimulationTimeline uninterrupted;
	REQUIRE(uninterrupted.advance(48));
	schedule_fixture(uninterrupted);

	const SimulationTimelineSnapshot checkpoint = uninterrupted.capture_snapshot();

	SimulationTimeline restored;
	REQUIRE(restored.restore_snapshot(checkpoint));

	const int64_t uninterrupted_digest = drain_digest_until(uninterrupted, 160);
	const int64_t restored_digest = drain_digest_until(restored, 160);

	CHECK(restored_digest == uninterrupted_digest);
	CHECK(restored.capture_snapshot() == uninterrupted.capture_snapshot());
	CHECK(restored.capture_snapshot().checksum() == uninterrupted.capture_snapshot().checksum());
}

TEST_CASE("Post-restore scheduling preserves sequence cursor", "[foundation][timeline][persistence][replay]") {
	SimulationTimeline original;
	schedule_fixture(original);
	const SimulationTimelineSnapshot checkpoint = original.capture_snapshot();

	SimulationTimeline restored;
	REQUIRE(restored.restore_snapshot(checkpoint));

	const auto a = original.schedule_event(
		SimTime::from_ticks(144),
		SimulationEventPayload { .type_id = "new.command", .data = { 42 } }
	);
	const auto b = restored.schedule_event(
		SimTime::from_ticks(144),
		SimulationEventPayload { .type_id = "new.command", .data = { 42 } }
	);

	REQUIRE(a.has_value());
	REQUIRE(b.has_value());
	CHECK(*a == *b);
	CHECK(restored.capture_snapshot() == original.capture_snapshot());
}