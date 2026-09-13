#pragma once

#include "openvic-simulation/core/simulation/SimTime.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006A4.1
 *
 * General actor-perception substrate:
 *
 * authoritative truth
 *   -> observation/report creation
 *   -> deterministic delivery delay
 *   -> actor knowledge
 *
 * This runtime does NOT own or mutate authoritative world state.
 * It does NOT implement military intelligence, sensors, espionage,
 * communications networks, or a universal noise model. Those are optional
 * producers/policies which may create reports for this substrate.
 */

enum class ObservationIntegrity : uint8_t {
	UNSPECIFIED = 0,
	DIRECT,
	ESTIMATED,
	DECEPTIVE
};

struct ObservationReport final {
	uint64_t sequence = 0;

	std::string recipient_actor_id;
	std::string source_id;
	std::string fact_type;
	std::string subject_id;

	SimTime observed_at = SimTime::from_ticks(0);
	SimTime deliver_at = SimTime::from_ticks(0);

	/*
	 * Opaque deterministic payload owned semantically by the producing domain.
	 * The perception substrate transports and stores it without interpreting it.
	 */
	std::vector<uint8_t> payload;

	ObservationIntegrity integrity = ObservationIntegrity::UNSPECIFIED;

	/*
	 * Integer basis-points confidence metadata: 0..10000.
	 * This is report metadata, not authoritative truth and not a universal
	 * probabilistic model.
	 */
	uint16_t confidence_basis_points = 0;

	bool operator==(ObservationReport const&) const = default;

	[[nodiscard]] bool is_valid() const {
		return !recipient_actor_id.empty()
			&& !source_id.empty()
			&& !fact_type.empty()
			&& !subject_id.empty()
			&& deliver_at.ticks() >= observed_at.ticks()
			&& confidence_basis_points <= 10'000;
	}
};

struct ActorKnowledgeRecord final {
	std::string actor_id;
	std::string fact_type;
	std::string subject_id;

	SimTime observed_at = SimTime::from_ticks(0);
	SimTime received_at = SimTime::from_ticks(0);

	std::string source_id;
	std::vector<uint8_t> payload;

	ObservationIntegrity integrity = ObservationIntegrity::UNSPECIFIED;
	uint16_t confidence_basis_points = 0;
	uint64_t source_report_sequence = 0;

	bool operator==(ActorKnowledgeRecord const&) const = default;
};

class ActorPerceptionRuntime final {
private:
	uint64_t next_sequence_value = 0;
	std::vector<ObservationReport> pending_reports;
	std::vector<ActorKnowledgeRecord> knowledge_records;

	[[nodiscard]] static bool report_order(
		ObservationReport const& lhs,
		ObservationReport const& rhs
	) {
		if (lhs.deliver_at.ticks() != rhs.deliver_at.ticks()) {
			return lhs.deliver_at.ticks() < rhs.deliver_at.ticks();
		}
		return lhs.sequence < rhs.sequence;
	}

	[[nodiscard]] static bool knowledge_key_less(
		ActorKnowledgeRecord const& lhs,
		ActorKnowledgeRecord const& rhs
	) {
		if (lhs.actor_id != rhs.actor_id) {
			return lhs.actor_id < rhs.actor_id;
		}
		if (lhs.fact_type != rhs.fact_type) {
			return lhs.fact_type < rhs.fact_type;
		}
		return lhs.subject_id < rhs.subject_id;
	}

	[[nodiscard]] static bool knowledge_key_less(
		ActorKnowledgeRecord const& lhs,
		std::tuple<std::string_view, std::string_view, std::string_view> rhs
	) {
		auto const [actor_id, fact_type, subject_id] = rhs;
		if (lhs.actor_id != actor_id) {
			return lhs.actor_id < actor_id;
		}
		if (lhs.fact_type != fact_type) {
			return lhs.fact_type < fact_type;
		}
		return lhs.subject_id < subject_id;
	}

	void apply_delivered_report(ObservationReport const& report, SimTime received_at) {
		ActorKnowledgeRecord incoming {
			.actor_id = report.recipient_actor_id,
			.fact_type = report.fact_type,
			.subject_id = report.subject_id,
			.observed_at = report.observed_at,
			.received_at = received_at,
			.source_id = report.source_id,
			.payload = report.payload,
			.integrity = report.integrity,
			.confidence_basis_points = report.confidence_basis_points,
			.source_report_sequence = report.sequence
		};

		auto const key = std::tuple {
			std::string_view { incoming.actor_id },
			std::string_view { incoming.fact_type },
			std::string_view { incoming.subject_id }
		};

		auto const iterator = std::lower_bound(
			knowledge_records.begin(),
			knowledge_records.end(),
			key,
			[](ActorKnowledgeRecord const& lhs, auto const& rhs) {
				return knowledge_key_less(lhs, rhs);
			}
		);

		if (
			iterator != knowledge_records.end()
			&& iterator->actor_id == incoming.actor_id
			&& iterator->fact_type == incoming.fact_type
			&& iterator->subject_id == incoming.subject_id
		) {
			/*
			 * Later-observed information supersedes earlier-observed information.
			 * Equal observation times resolve by report sequence, preserving
			 * deterministic delivery order.
			 */
			if (
				incoming.observed_at.ticks() > iterator->observed_at.ticks()
				|| (
					incoming.observed_at == iterator->observed_at
					&& incoming.source_report_sequence > iterator->source_report_sequence
				)
			) {
				*iterator = std::move(incoming);
			}
			return;
		}

		knowledge_records.insert(iterator, std::move(incoming));
	}

public:
	[[nodiscard]] std::optional<uint64_t> submit_report(ObservationReport report) {
		if (!report.is_valid()) {
			return std::nullopt;
		}

		report.sequence = next_sequence_value++;

		auto const iterator = std::lower_bound(
			pending_reports.begin(),
			pending_reports.end(),
			report,
			[](ObservationReport const& lhs, ObservationReport const& rhs) {
				return report_order(lhs, rhs);
			}
		);
		pending_reports.insert(iterator, std::move(report));
		return next_sequence_value - 1;
	}

	[[nodiscard]] std::size_t deliver_due(SimTime now) {
		std::size_t delivered = 0;

		while (
			!pending_reports.empty()
			&& pending_reports.front().deliver_at.ticks() <= now.ticks()
		) {
			ObservationReport report = std::move(pending_reports.front());
			pending_reports.erase(pending_reports.begin());

			apply_delivered_report(report, now);
			++delivered;
		}

		return delivered;
	}

	[[nodiscard]] ActorKnowledgeRecord const* find_knowledge(
		std::string_view actor_id,
		std::string_view fact_type,
		std::string_view subject_id
	) const {
		auto const key = std::tuple { actor_id, fact_type, subject_id };

		auto const iterator = std::lower_bound(
			knowledge_records.begin(),
			knowledge_records.end(),
			key,
			[](ActorKnowledgeRecord const& lhs, auto const& rhs) {
				return knowledge_key_less(lhs, rhs);
			}
		);

		if (
			iterator == knowledge_records.end()
			|| iterator->actor_id != actor_id
			|| iterator->fact_type != fact_type
			|| iterator->subject_id != subject_id
		) {
			return nullptr;
		}

		return &*iterator;
	}

	[[nodiscard]] std::size_t pending_report_count() const {
		return pending_reports.size();
	}

	[[nodiscard]] std::size_t knowledge_record_count() const {
		return knowledge_records.size();
	}

	[[nodiscard]] uint64_t next_sequence() const {
		return next_sequence_value;
	}

	[[nodiscard]] std::span<ObservationReport const> pending() const {
		return pending_reports;
	}

	[[nodiscard]] std::span<ActorKnowledgeRecord const> knowledge() const {
		return knowledge_records;
	}
};

}