#include "openvic-simulation/core/simulation/Cadence.hpp"
#include "openvic-simulation/core/simulation/SimulationEventScheduler.hpp"
#include "openvic-simulation/core/simulation/SimTime.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("SimTime is unitless and overflow-safe", "[foundation][time]") {
	const SimTime start = SimTime::from_ticks(10);
	REQUIRE(start.checked_advance(5).has_value());
	CHECK(start.checked_advance(5)->ticks() == 15);
	CHECK_FALSE(SimTime::from_ticks(std::numeric_limits<int64_t>::max()).checked_advance(1).has_value());
	CHECK_FALSE(SimTime::from_ticks(std::numeric_limits<int64_t>::min()).checked_advance(-1).has_value());
}

TEST_CASE("Cadence validates explicit periods and phases", "[foundation][cadence]") {
	CHECK_FALSE(Cadence::create(0, 0).has_value());
	CHECK_FALSE(Cadence::create(24, -1).has_value());
	CHECK_FALSE(Cadence::create(24, 24).has_value());

	const auto cadence = Cadence::create(24, 5);
	REQUIRE(cadence.has_value());
	CHECK(cadence->is_due(SimTime::from_ticks(5)));
	CHECK(cadence->is_due(SimTime::from_ticks(29)));
	CHECK_FALSE(cadence->is_due(SimTime::from_ticks(24)));
	REQUIRE(cadence->next_at_or_after(SimTime::from_ticks(24)).has_value());
	CHECK(cadence->next_at_or_after(SimTime::from_ticks(24))->ticks() == 29);
}

TEST_CASE("Staggered cadence is stable and domain-specific", "[foundation][cadence]") {
	const auto population_a = Cadence::staggered(24, 42, "population");
	const auto population_b = Cadence::staggered(24, 42, "population");
	const auto firms = Cadence::staggered(24, 42, "firms");

	REQUIRE(population_a.has_value());
	REQUIRE(population_b.has_value());
	REQUIRE(firms.has_value());

	CHECK(population_a->phase_ticks() == population_b->phase_ticks());
	CHECK(population_a->phase_ticks() != firms->phase_ticks());
	CHECK_FALSE(Cadence::staggered(24, 42, "").has_value());
}

TEST_CASE("Equal-time scheduled events preserve insertion sequence", "[foundation][events]") {
	SimulationEventScheduler scheduler;

	REQUIRE(scheduler.schedule(
		SimTime::from_ticks(10),
		SimulationEventPayload { .type_id = "first", .data = {} }
	).has_value());
	REQUIRE(scheduler.schedule(
		SimTime::from_ticks(10),
		SimulationEventPayload { .type_id = "second", .data = {} }
	).has_value());

	const auto first = scheduler.pop_due(SimTime::from_ticks(10));
	const auto second = scheduler.pop_due(SimTime::from_ticks(10));
	REQUIRE(first.has_value());
	REQUIRE(second.has_value());
	CHECK(first->payload.type_id == "first");
	CHECK(second->payload.type_id == "second");
	CHECK(first->sequence < second->sequence);
}

TEST_CASE("Selective event wake-up does not disturb unrelated events", "[foundation][events]") {
	SimulationEventScheduler scheduler;

	const SimulationEventPayload unrelated { .type_id = "unrelated", .data = { 1 } };
	const SimulationEventPayload communications { .type_id = "communications", .data = { 2 } };

	REQUIRE(scheduler.schedule(SimTime::from_ticks(4), unrelated).has_value());
	REQUIRE(scheduler.schedule(SimTime::from_ticks(5), communications).has_value());
	CHECK(scheduler.has_due_type(SimTime::from_ticks(5), "communications"));

	const auto selected = scheduler.pop_due_type(SimTime::from_ticks(5), "communications");
	REQUIRE(selected.has_value());
	CHECK(selected->payload == communications);

	const auto remaining = scheduler.pop_due(SimTime::from_ticks(5));
	REQUIRE(remaining.has_value());
	CHECK(remaining->payload == unrelated);
}

TEST_CASE("Event scheduler snapshot restore preserves canonical order", "[foundation][events][persistence]") {
	SimulationEventScheduler scheduler;
	REQUIRE(scheduler.schedule(
		SimTime::from_ticks(7),
		SimulationEventPayload { .type_id = "a", .data = { 1, 2 } }
	).has_value());
	REQUIRE(scheduler.schedule(
		SimTime::from_ticks(7),
		SimulationEventPayload { .type_id = "b", .data = { 3 } }
	).has_value());
	REQUIRE(scheduler.schedule(
		SimTime::from_ticks(9),
		SimulationEventPayload { .type_id = "c", .data = {} }
	).has_value());

	const auto snapshot = scheduler.snapshot_events();
	const auto restored = SimulationEventScheduler::restore(scheduler.next_sequence(), snapshot);
	REQUIRE(restored.has_value());
	CHECK(restored->next_sequence() == scheduler.next_sequence());
	CHECK(restored->snapshot_events() == snapshot);

	auto noncanonical = snapshot;
	std::swap(noncanonical[0], noncanonical[1]);
	CHECK_FALSE(SimulationEventScheduler::restore(scheduler.next_sequence(), noncanonical).has_value());
}