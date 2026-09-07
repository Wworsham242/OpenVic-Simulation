#pragma once

#include <span>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

class AggregateProducer;
struct Pop;

/// One allocation pass after the caller's daily POP employment reset and before
/// production. The caller supplies the local labor pool in deterministic order
/// and coordinates competing employers. Only unemployed, job-eligible workers
/// are hired, up to installed capacity. Job effects/ratios are not hiring quotas.
/// No employment ownership or cross-day ledger is retained here.
[[nodiscard]] fixed_point_t allocate_producer_workforce(
	AggregateProducer& producer, std::span<Pop> pops
);

}
