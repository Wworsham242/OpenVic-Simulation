#include "openvic-simulation/core/simulation/ActorPerceptionRuntime.hpp"

#include <snitch/snitch.hpp>

#include <cstdint>
#include <vector>

using namespace OpenVic;

TEST_CASE(
	"Bronze messenger report is delayed and recipient-limited",
	"[convergence][actor-perception][bronze]"
) {
	ActorPerceptionRuntime runtime;

	/*
	 * Test-owned authoritative truth. The perception runtime never receives a
	 * mutable reference to it.
	 */
	int64_t const authoritative_grain_store = 820;

	ObservationReport report {
		.recipient_actor_id = "palace.administrator",
		.source_id = "messenger.village-17",
		.fact_type = "grain.store",
		.subject_id = "village-17",
		.observed_at = SimTime::from_ticks(24),
		.deliver_at = SimTime::from_ticks(24 + (5 * 24)),
		.payload = { 0x03, 0x34 },
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 8'500
	};

	auto const sequence = runtime.submit_report(std::move(report));
	REQUIRE(sequence.has_value());
	CHECK(*sequence == 0);
	CHECK(runtime.pending_report_count() == 1);

	CHECK(runtime.deliver_due(SimTime::from_ticks(24 + (4 * 24))) == 0);
	CHECK(
		runtime.find_knowledge(
			"palace.administrator",
			"grain.store",
			"village-17"
		) == nullptr
	);

	CHECK(runtime.deliver_due(SimTime::from_ticks(24 + (5 * 24))) == 1);

	ActorKnowledgeRecord const* const palace_knowledge =
		runtime.find_knowledge(
			"palace.administrator",
			"grain.store",
			"village-17"
		);

	REQUIRE(palace_knowledge != nullptr);
	CHECK(palace_knowledge->observed_at == SimTime::from_ticks(24));
	CHECK(
		palace_knowledge->received_at
			== SimTime::from_ticks(24 + (5 * 24))
	);
	CHECK(palace_knowledge->integrity == ObservationIntegrity::DIRECT);
	CHECK(palace_knowledge->confidence_basis_points == 8'500);

	/* Another actor receives no implicit omniscient copy. */
	CHECK(
		runtime.find_knowledge(
			"temple.administrator",
			"grain.store",
			"village-17"
		) == nullptr
	);

	CHECK(authoritative_grain_store == 820);
}

TEST_CASE(
	"Modern report can arrive quickly with uncertainty or deception metadata",
	"[convergence][actor-perception][modern]"
) {
	ActorPerceptionRuntime runtime;

	int64_t const authoritative_vehicle_count = 72;

	ObservationReport estimated {
		.recipient_actor_id = "ministry.analysis-cell",
		.source_id = "sensor.constellation-a",
		.fact_type = "formation.vehicle-count",
		.subject_id = "formation-red-4",
		.observed_at = SimTime::from_ticks(1'000),
		.deliver_at = SimTime::from_ticks(1'002),
		.payload = { 68 },
		.integrity = ObservationIntegrity::ESTIMATED,
		.confidence_basis_points = 7'200
	};

	REQUIRE(runtime.submit_report(std::move(estimated)).has_value());
	CHECK(runtime.deliver_due(SimTime::from_ticks(1'001)) == 0);
	CHECK(runtime.deliver_due(SimTime::from_ticks(1'002)) == 1);

	ActorKnowledgeRecord const* knowledge =
		runtime.find_knowledge(
			"ministry.analysis-cell",
			"formation.vehicle-count",
			"formation-red-4"
		);
	REQUIRE(knowledge != nullptr);
	CHECK(knowledge->payload == std::vector<uint8_t> { 68 });
	CHECK(knowledge->integrity == ObservationIntegrity::ESTIMATED);
	CHECK(knowledge->confidence_basis_points == 7'200);

	/*
	 * A later deceptive report is still only perceived information. The
	 * substrate records provenance/integrity metadata but does not decide
	 * whether the actor believes it and never edits authoritative truth.
	 */
	ObservationReport deceptive {
		.recipient_actor_id = "ministry.analysis-cell",
		.source_id = "foreign.broadcast",
		.fact_type = "formation.vehicle-count",
		.subject_id = "formation-red-4",
		.observed_at = SimTime::from_ticks(1'003),
		.deliver_at = SimTime::from_ticks(1'004),
		.payload = { 41 },
		.integrity = ObservationIntegrity::DECEPTIVE,
		.confidence_basis_points = 4'000
	};

	REQUIRE(runtime.submit_report(std::move(deceptive)).has_value());
	CHECK(runtime.deliver_due(SimTime::from_ticks(1'004)) == 1);

	knowledge = runtime.find_knowledge(
		"ministry.analysis-cell",
		"formation.vehicle-count",
		"formation-red-4"
	);
	REQUIRE(knowledge != nullptr);
	CHECK(knowledge->payload == std::vector<uint8_t> { 41 });
	CHECK(knowledge->integrity == ObservationIntegrity::DECEPTIVE);
	CHECK(authoritative_vehicle_count == 72);
}

TEST_CASE(
	"Actor perception delivery is deterministic and stale reports do not overwrite newer knowledge",
	"[convergence][actor-perception][determinism]"
) {
	ActorPerceptionRuntime runtime;

	ObservationReport newer {
		.recipient_actor_id = "actor.a",
		.source_id = "source.fast",
		.fact_type = "price.index",
		.subject_id = "market.1",
		.observed_at = SimTime::from_ticks(20),
		.deliver_at = SimTime::from_ticks(25),
		.payload = { 20 },
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 9'000
	};

	ObservationReport stale {
		.recipient_actor_id = "actor.a",
		.source_id = "source.slow",
		.fact_type = "price.index",
		.subject_id = "market.1",
		.observed_at = SimTime::from_ticks(10),
		.deliver_at = SimTime::from_ticks(30),
		.payload = { 10 },
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 9'000
	};

	REQUIRE(runtime.submit_report(std::move(stale)).has_value());
	REQUIRE(runtime.submit_report(std::move(newer)).has_value());

	CHECK(runtime.pending().size() == 2);
	CHECK(runtime.pending()[0].deliver_at == SimTime::from_ticks(25));
	CHECK(runtime.pending()[1].deliver_at == SimTime::from_ticks(30));

	CHECK(runtime.deliver_due(SimTime::from_ticks(25)) == 1);

	ActorKnowledgeRecord const* knowledge =
		runtime.find_knowledge("actor.a", "price.index", "market.1");
	REQUIRE(knowledge != nullptr);
	CHECK(knowledge->payload == std::vector<uint8_t> { 20 });
	CHECK(knowledge->observed_at == SimTime::from_ticks(20));

	CHECK(runtime.deliver_due(SimTime::from_ticks(30)) == 1);

	knowledge =
		runtime.find_knowledge("actor.a", "price.index", "market.1");
	REQUIRE(knowledge != nullptr);

	/* Slow stale information must not roll actor knowledge backwards. */
	CHECK(knowledge->payload == std::vector<uint8_t> { 20 });
	CHECK(knowledge->observed_at == SimTime::from_ticks(20));
	CHECK(runtime.pending_report_count() == 0);
	CHECK(runtime.knowledge_record_count() == 1);
}

TEST_CASE(
	"Actor perception rejects invalid report envelopes",
	"[convergence][actor-perception][validation]"
) {
	ActorPerceptionRuntime runtime;

	ObservationReport invalid {
		.recipient_actor_id = "",
		.source_id = "source",
		.fact_type = "fact",
		.subject_id = "subject",
		.observed_at = SimTime::from_ticks(10),
		.deliver_at = SimTime::from_ticks(9),
		.payload = {},
		.integrity = ObservationIntegrity::UNSPECIFIED,
		.confidence_basis_points = 10'001
	};

	CHECK_FALSE(runtime.submit_report(std::move(invalid)).has_value());
	CHECK(runtime.pending_report_count() == 0);
	CHECK(runtime.knowledge_record_count() == 0);
}