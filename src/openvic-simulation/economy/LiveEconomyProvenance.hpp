#pragma once

#include <optional>
#include <string>

#include "openvic-simulation/core/simulation/SimTime.hpp"
#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"
#include "openvic-simulation/economy/production/WorkforceAllocation.hpp"
#include "openvic-simulation/economy/trading/DeliverableSupply.hpp"
#include "openvic-simulation/resources/ResourceSupplyNetwork.hpp"

namespace OpenVic {

// One fixed vertical in execution order, assembled from native domain facts.
// No graph, event history, pointers, or inferred external causes. Constraint
// flags describe their own stage and can coexist (e.g. labor and input ceilings).
struct LiveEconomyCycleProvenance final {
	uint64_t cycle = 0;
	// Direct compatibility pre/post calls have no scheduled due time.
	std::optional<SimTime> due_time;
	std::string upstream_process_id;
	std::string downstream_process_id;
	std::string intermediate_good_id;
	ResourceFlowResult resource;
	// Absent when no native allocation occurred (including manual workforce).
	std::optional<WorkforceAllocationResult> workforce;
	AggregateProductionResult upstream;
	DeliverableSupply logistics;
	AggregateMarketCycleResult upstream_market;
	AggregateMarketCycleResult downstream_market;
	fixed_point_t market_price = 0;
	AggregateProductionResult downstream;

	bool operator==(LiveEconomyCycleProvenance const&) const = default;
};

}
