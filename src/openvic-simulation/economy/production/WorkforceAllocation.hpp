#pragma once

#include <span>
#include <variant>
#include "openvic-simulation/core/memory/Colony.hpp"

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

class AggregateProducer;
struct Pop;

// Non-owning adapters over authoritative storage, never another POP container.
struct WorkforceColonyView { memory::colony<Pop>& pops; };
using WorkforcePool = std::variant<std::span<Pop>, WorkforceColonyView>;

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

[[nodiscard]] fixed_point_t allocate_producer_workforce_from_pool(
	AggregateProducer& producer, WorkforcePool const& pool, WorkforceAllocationResult* result = nullptr
);

}
