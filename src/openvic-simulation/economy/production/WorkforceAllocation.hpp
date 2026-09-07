#pragma once

#include <span>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

class AggregateProducer;
struct Pop;

struct WorkforceAllocationResult final {
	fixed_point_t requested = 0;
	fixed_point_t allocated = 0;
	bool operator==(WorkforceAllocationResult const&) const = default;
};

/// One allocation pass after the caller's daily POP employment reset and before
/// production. The caller supplies the local labor pool in deterministic order
/// and coordinates competing employers. Only unemployed, job-eligible workers
/// are hired, up to installed capacity. Job effects/ratios are not hiring quotas.
/// No employment ownership or cross-day ledger is retained here.
[[nodiscard]] fixed_point_t allocate_producer_workforce(
	AggregateProducer& producer, std::span<Pop> pops,
	WorkforceAllocationResult* result = nullptr
);

}
