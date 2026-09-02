#include "openvic-simulation/core/simulation/Cadence.hpp"
#include "openvic-simulation/core/simulation/SimTime.hpp"
#include "openvic-simulation/core/simulation/SimulationEventScheduler.hpp"
#include "openvic-simulation/ecs/Checksum.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <array>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ecs;

namespace {
	struct IntegrationClock {
		int64_t tick = 0;
	};

	struct IntegrationEventSignal {
		int64_t delivered_count = 0;
		int64_t ordered_digest = 0;
	};

	struct IntegrationPeriodicValue {
		int64_t value = 0;
	};

	struct IntegrationEventValue {
		int64_t value = 0;
	};
}

ECS_COMPONENT(IntegrationClock, "test_FOUNDATION002::Clock")
ECS_COMPONENT(IntegrationEventSignal, "test_FOUNDATION002::EventSignal")
ECS_COMPONENT(IntegrationPeriodicValue, "test_FOUNDATION002::PeriodicValue")
ECS_COMPONENT(IntegrationEventValue, "test_FOUNDATION002::EventValue")

namespace {
	struct PeriodicSystem : SystemThreaded<PeriodicSystem> {
		static bool should_run(TickContext const& ctx) {
			IntegrationClock const* clock = ctx.world.get_singleton<IntegrationClock>();
			if (clock == nullptr) {
				return false;
			}

			static constexpr auto cadence = Cadence::create(6, 2);
			static_assert(cadence.has_value());
			return cadence->is_due(SimTime::from_ticks(clock->tick));
		}

		void tick(TickContext const&, IntegrationPeriodicValue& value) {
			value.value = value.value * 31 + 7;
		}
	};

	struct EventObservationSystem : System<EventObservationSystem> {
		static constexpr std::array<component_type_id_t, 1> extra_reads() {
			return { component_type_id_of<IntegrationEventSignal>() };
		}

		void tick(TickContext const& ctx, IntegrationEventValue& value) {
			IntegrationEventSignal const* signal = ctx.world.get_singleton<IntegrationEventSignal>();
			if (signal == nullptr) {
				return;
			}
			value.value =
				value.value * 1000003
				+ signal->delivered_count * 97
				+ signal->ordered_digest;
		}
	};
}

ECS_SYSTEM(PeriodicSystem)
ECS_SYSTEM(EventObservationSystem)

namespace {
	struct RunResult {
		uint64_t schedule_hash = 0;
		uint64_t world_checksum = 0;
		int64_t periodic_digest = 0;
		int64_t event_digest = 0;
		int64_t delivered_count = 0;

		bool operator==(RunResult const&) const = default;
	};

	void deliver_due_events(
		SimulationEventScheduler& scheduler,
		SimTime now,
		IntegrationEventSignal& signal
	) {
		while (auto event = scheduler.pop_due(now)) {
			signal.delivered_count += 1;
			// Digest sequence and first payload byte so same-time ordering is observable.
			const int64_t byte =
				event->payload.data.empty() ? 0 : static_cast<int64_t>(event->payload.data.front());
			signal.ordered_digest =
				signal.ordered_digest * 257
				+ static_cast<int64_t>(event->sequence + 1) * 17
				+ byte;
		}
	}

	RunResult run_integration(uint32_t worker_count, bool serial_mode, int64_t start_tick = 0) {
		World world;
		world.set_ecs_worker_count(worker_count);
		world.set_serial_mode(serial_mode);

		IntegrationClock* clock = world.set_singleton(IntegrationClock { start_tick });
		IntegrationEventSignal* signal = world.set_singleton(IntegrationEventSignal {});
		REQUIRE(clock != nullptr);
		REQUIRE(signal != nullptr);

		std::vector<EntityID> ids;
		for (int64_t i = 0; i < 128; ++i) {
			ids.push_back(world.create_entity(
				IntegrationPeriodicValue { i + 1 },
				IntegrationEventValue { i * 3 + 5 }
			));
		}

		world.register_system<PeriodicSystem>();
		world.register_system<EventObservationSystem>();

		const uint64_t initial_schedule_hash = world.schedule_hash();
		REQUIRE(initial_schedule_hash != 0);

		SimulationEventScheduler events;
		REQUIRE(events.schedule(
			SimTime::from_ticks(start_tick + 5),
			SimulationEventPayload { .type_id = "alpha", .data = { 11 } }
		).has_value());
		REQUIRE(events.schedule(
			SimTime::from_ticks(start_tick + 5),
			SimulationEventPayload { .type_id = "beta", .data = { 22 } }
		).has_value());
		REQUIRE(events.schedule(
			SimTime::from_ticks(start_tick + 17),
			SimulationEventPayload { .type_id = "gamma", .data = { 33 } }
		).has_value());

		for (int64_t step = 0; step < 48; ++step) {
			const SimTime now = SimTime::from_ticks(start_tick + step);
			clock->tick = now.ticks();
			deliver_due_events(events, now, *signal);

			// Date remains a compatibility argument to the existing ECS API.
			// FOUNDATION-002 proves the new generalized timing layer can gate
			// systems without changing the ECS schedule or Date machinery yet.
			world.tick_systems(Date {});
			REQUIRE(world.schedule_hash() == initial_schedule_hash);
		}

		RunResult result;
		result.schedule_hash = world.schedule_hash();
		result.world_checksum = world_checksum(world);
		result.delivered_count = signal->delivered_count;

		for (EntityID id : ids) {
			auto const* periodic = world.get_component<IntegrationPeriodicValue>(id);
			auto const* event_value = world.get_component<IntegrationEventValue>(id);
			REQUIRE(periodic != nullptr);
			REQUIRE(event_value != nullptr);
			result.periodic_digest = result.periodic_digest * 1000003 + periodic->value;
			result.event_digest = result.event_digest * 1000033 + event_value->value;
		}

		return result;
	}
}

TEST_CASE(
	"Generalized cadence drives ECS should_run without changing schedule topology",
	"[foundation][integration][ecs][cadence]"
) {
	World first;
	first.set_singleton(IntegrationClock { 0 });
	first.create_entity(IntegrationPeriodicValue {});
	first.register_system<PeriodicSystem>();

	World second;
	second.set_singleton(IntegrationClock { 3 });
	second.create_entity(IntegrationPeriodicValue {});
	second.register_system<PeriodicSystem>();

	const uint64_t first_hash = first.schedule_hash();
	const uint64_t second_hash = second.schedule_hash();

	REQUIRE(first_hash != 0);
	CHECK(first_hash == second_hash);

	// Tick 0 is not cadence phase 2; tick 2 is.
	first.tick_systems(Date {});
	// Avoid relying on guessed EntityID construction below; verify through iteration instead.
	int64_t observed = -1;
	first.for_each<IntegrationPeriodicValue>([&](IntegrationPeriodicValue& value) {
		observed = value.value;
	});
	CHECK(observed == 0);

	first.get_singleton<IntegrationClock>()->tick = 2;
	first.tick_systems(Date {});
	observed = -1;
	first.for_each<IntegrationPeriodicValue>([&](IntegrationPeriodicValue& value) {
		observed = value.value;
	});
	CHECK(observed == 7);
	CHECK(first.schedule_hash() == first_hash);
}

TEST_CASE(
	"Timing and event integration is worker-count invariant",
	"[foundation][integration][ecs][determinism]"
) {
	const RunResult baseline = run_integration(1, false);

	for (uint32_t worker_count : { 1u, 2u, 4u, 8u, 16u }) {
		CHECK(run_integration(worker_count, false) == baseline);
	}
}

TEST_CASE(
	"Timing and event integration is serial-parallel invariant",
	"[foundation][integration][ecs][determinism]"
) {
	const RunResult parallel = run_integration(8, false);
	const RunResult serial = run_integration(1, true);

	CHECK(serial == parallel);
	CHECK(parallel.delivered_count == 3);
}

TEST_CASE(
	"Event snapshot restore produces identical future ECS result",
	"[foundation][integration][events][restore]"
) {
	SimulationEventScheduler original;
	REQUIRE(original.schedule(
		SimTime::from_ticks(3),
		SimulationEventPayload { .type_id = "first", .data = { 4 } }
	).has_value());
	REQUIRE(original.schedule(
		SimTime::from_ticks(3),
		SimulationEventPayload { .type_id = "second", .data = { 9 } }
	).has_value());
	REQUIRE(original.schedule(
		SimTime::from_ticks(11),
		SimulationEventPayload { .type_id = "third", .data = { 16 } }
	).has_value());

	const auto snapshot = original.snapshot_events();
	auto restored = SimulationEventScheduler::restore(original.next_sequence(), snapshot);
	REQUIRE(restored.has_value());

	IntegrationEventSignal first_signal {};
	IntegrationEventSignal restored_signal {};

	for (int64_t tick = 0; tick < 20; ++tick) {
		const SimTime now = SimTime::from_ticks(tick);
		deliver_due_events(original, now, first_signal);
		deliver_due_events(*restored, now, restored_signal);
	}

	CHECK(first_signal.delivered_count == restored_signal.delivered_count);
	CHECK(first_signal.ordered_digest == restored_signal.ordered_digest);
	CHECK(original.empty());
	CHECK(restored->empty());
}