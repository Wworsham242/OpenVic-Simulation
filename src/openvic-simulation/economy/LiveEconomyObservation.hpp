#pragma once

#include "openvic-simulation/core/simulation/ActorPerceptionRuntime.hpp"
#include "openvic-simulation/economy/LiveEconomyProvenance.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006A4.3
 *
 * Economy-owned observation adapter.
 *
 * The economy decides which completed authoritative fact is reportable and
 * constructs the report envelope. ActorPerceptionRuntime only transports and
 * stores the resulting perceived information.
 *
 * This first proof intentionally exposes one narrow fact:
 * whether the authoritative upstream market transaction was supply-limited
 * during the completed live-economy cycle.
 */
struct LiveEconomyObservationPolicy final {
	std::string recipient_actor_id;
	int64_t delivery_delay_ticks = 24;

	[[nodiscard]] bool is_valid() const {
		return !recipient_actor_id.empty() && delivery_delay_ticks >= 0;
	}
};

[[nodiscard]] inline std::optional<ObservationReport>
make_live_economy_transaction_limit_report(
	LiveEconomyCycleProvenance const& provenance,
	LiveEconomyObservationPolicy const& policy
) {
	if (!policy.is_valid() || !provenance.due_time.has_value()) {
		return std::nullopt;
	}

	SimTime const observed_at = *provenance.due_time;
	int64_t const observed_ticks = observed_at.ticks();

	if (
		policy.delivery_delay_ticks > 0
		&& observed_ticks
			> std::numeric_limits<int64_t>::max()
				- policy.delivery_delay_ticks
	) {
		return std::nullopt;
	}

	bool const transaction_limited =
		provenance.upstream_market.transaction_limited();

	return ObservationReport {
		.recipient_actor_id = policy.recipient_actor_id,
		.source_id = "economy.live-cycle",
		.fact_type = "economy.market.transaction-limited",
		.subject_id = provenance.intermediate_good_id,
		.observed_at = observed_at,
		.deliver_at = SimTime::from_ticks(
			observed_ticks + policy.delivery_delay_ticks
		),
		.payload = std::vector<uint8_t> {
			static_cast<uint8_t>(transaction_limited ? 1 : 0)
		},
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 10'000
	};
}

}