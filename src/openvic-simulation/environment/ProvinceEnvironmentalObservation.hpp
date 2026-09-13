#pragma once

#include "openvic-simulation/core/simulation/ActorPerceptionRuntime.hpp"
#include "openvic-simulation/environment/ProvinceEnvironmentalState.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006A4.4
 *
 * Environment-owned observation adapter.
 *
 * This adapter reports one physical authoritative fact owned by
 * ProvinceEnvironmentalState. It does not infer agricultural output, weather,
 * climate, irrigation, or any other unimplemented environmental mechanism.
 */
struct ProvinceEnvironmentalObservationPolicy final {
	std::string recipient_actor_id;
	int64_t delivery_delay_ticks = 24;

	[[nodiscard]] bool is_valid() const {
		return !recipient_actor_id.empty() && delivery_delay_ticks >= 0;
	}
};

[[nodiscard]] inline std::optional<ObservationReport>
make_province_water_availability_report(
	std::string province_id,
	ProvinceEnvironmentalState const& environment,
	SimTime observed_at,
	ProvinceEnvironmentalObservationPolicy const& policy
) {
	if (province_id.empty() || !policy.is_valid()) {
		return std::nullopt;
	}

	int64_t const observed_ticks = observed_at.ticks();
	if (
		policy.delivery_delay_ticks > 0
		&& observed_ticks
			> std::numeric_limits<int64_t>::max()
				- policy.delivery_delay_ticks
	) {
		return std::nullopt;
	}

	/*
	 * Encode fixed_point_t raw value explicitly so actor knowledge receives the
	 * exact domain value without adding a second environmental representation.
	 */
	auto const raw = environment.get_water_availability().get_raw_value();

	std::vector<uint8_t> payload(sizeof(raw));
	for (std::size_t i = 0; i < sizeof(raw); ++i) {
		payload[i] = static_cast<uint8_t>(
			(static_cast<uint64_t>(raw) >> (i * 8)) & 0xFFu
		);
	}

	return ObservationReport {
		.recipient_actor_id = policy.recipient_actor_id,
		.source_id = "environment.province",
		.fact_type = "environment.water-availability",
		.subject_id = std::move(province_id),
		.observed_at = observed_at,
		.deliver_at = SimTime::from_ticks(
			observed_ticks + policy.delivery_delay_ticks
		),
		.payload = std::move(payload),
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 10'000
	};
}

}