#include "WorkforceAllocation.hpp"

#include <algorithm>
#include <ranges>

#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperation.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopType.hpp"

using namespace OpenVic;

template<typename Pops>
static fixed_point_t allocate_workforce(
	AggregateProducer& producer, Pops&& pops, WorkforceAllocationResult* result
) {
	ProductionType const& process = producer.get_production_type();
	fixed_point_t remaining = producer.get_capacity()
		* fixed_point_t { type_safe::get(process.base_workforce_size) };
	fixed_point_t allocated = 0;
	if (result != nullptr) {
		*result = WorkforceAllocationResult { .requested = remaining };
	}

	for (Pop& pop : pops) {
		if (remaining < fixed_point_t::_1) {
			break;
		}
		bool const eligible = std::ranges::any_of(process.get_jobs(), [&pop](Job const& job) {
			return job.pop_type_index == pop.get_type().index;
		});
		if (!eligible || pop.get_unemployed() <= 0) {
			continue;
		}
		// Bound by this POP before converting, so a large facility's total
		// demand never has to fit in the per-POP integer size type.
		pop_size_t const count = std::min(
			remaining, fixed_point_t { type_safe::get(pop.get_unemployed()) }
		).floor<type_safe::underlying_type<pop_size_t>>();
		pop.hire(count);
		fixed_point_t const hired { type_safe::get(count) };
		allocated += hired;
		remaining -= hired;
	}
	producer.set_available_workforce(allocated);
	if (result != nullptr) {
		result->allocated = allocated;
	}
	return allocated;
}

fixed_point_t OpenVic::allocate_producer_workforce(
	AggregateProducer& producer, std::span<Pop> pops, WorkforceAllocationResult* result
) {
	return allocate_workforce(producer, pops, result);
}

fixed_point_t OpenVic::allocate_producer_workforce_from_pool(
	AggregateProducer& producer, WorkforcePool const& pool, WorkforceAllocationResult* result
) {
	if (auto const* span = std::get_if<std::span<Pop>>(&pool)) {
		return allocate_workforce(producer, *span, result);
	}
	return allocate_workforce(producer, std::get<WorkforceColonyView>(pool).pops, result);
}

namespace {

template<typename Pops>
std::vector<WorkforceEmployerAllocation> allocate_competing_employers_impl(
std::vector<WorkforceEmployerRequest> requests,
Pops&& pops
) {
std::ranges::stable_sort(
requests,
[](WorkforceEmployerRequest const& lhs, WorkforceEmployerRequest const& rhs) {
if (lhs.labor_offer != rhs.labor_offer) {
return lhs.labor_offer > rhs.labor_offer;
}
return lhs.employer_id < rhs.employer_id;
}
);

std::vector<WorkforceEmployerAllocation> results;
results.reserve(requests.size());

for (WorkforceEmployerRequest const& request : requests) {
WorkforceEmployerAllocation result {
.employer_id = request.employer_id,
.requested = std::max(request.requested, fixed_point_t::_0),
.allocated = fixed_point_t::_0
};

if (
result.requested < fixed_point_t::_1 ||
request.employer == nullptr ||
request.accepts == nullptr ||
request.assign == nullptr
) {
results.push_back(result);
continue;
}

fixed_point_t remaining = result.requested;

for (Pop& pop : pops) {
if (remaining < fixed_point_t::_1) {
break;
}

if (
pop.get_unemployed() <= 0 ||
!request.accepts(request.employer, pop)
) {
continue;
}

pop_size_t const available = pop.get_unemployed();

pop_size_t const desired = std::min(
remaining,
fixed_point_t { type_safe::get(available) }
).floor<type_safe::underlying_type<pop_size_t>>();

if (desired <= 0) {
continue;
}

pop_size_t const assigned =
request.assign(request.employer, pop, desired);

if (assigned <= 0) {
continue;
}

fixed_point_t const assigned_fp {
type_safe::get(assigned)
};

result.allocated += assigned_fp;
remaining -= assigned_fp;
}

results.push_back(result);
}

// Results are returned in deterministic allocation order, which also makes
// provenance/debugging explicit about which policy won contested workers.
return results;
}

}

std::vector<WorkforceEmployerAllocation> OpenVic::allocate_competing_employers(
std::vector<WorkforceEmployerRequest> requests,
WorkforcePool const& pool
) {
if (auto const* span = std::get_if<std::span<Pop>>(&pool)) {
return allocate_competing_employers_impl(
std::move(requests),
*span
);
}

return allocate_competing_employers_impl(
std::move(requests),
std::get<WorkforceColonyView>(pool).pops
);
}
namespace {

bool rgo_accepts_worker_adapter(void* employer, Pop const& pop) {
return static_cast<ResourceGatheringOperation*>(employer)->accepts_worker(pop);
}

pop_size_t rgo_assign_worker_adapter(
void* employer,
Pop& pop,
pop_size_t requested
) {
return static_cast<ResourceGatheringOperation*>(employer)->assign_worker(
pop,
requested
);
}

bool producer_accepts_worker_adapter(void* employer, Pop const& pop) {
auto const& producer = *static_cast<AggregateProducer*>(employer);
auto const& process = producer.get_production_type();

return std::ranges::any_of(
process.get_jobs(),
[&pop](Job const& job) {
return job.pop_type_index == pop.get_type().index;
}
);
}

pop_size_t producer_assign_worker_adapter(
void* employer,
Pop& pop,
pop_size_t requested
) {
auto& producer = *static_cast<AggregateProducer*>(employer);

pop_size_t const available = pop.get_unemployed();
pop_size_t const actual = std::min(requested, available);

if (actual <= 0) {
return pop_size_t { 0 };
}

pop.hire(actual);

producer.set_available_workforce(
producer.get_available_workforce()
+ fixed_point_t { type_safe::get(actual) }
);

return actual;
}

}

WorkforceEmployerRequest OpenVic::make_rgo_workforce_request(
ResourceGatheringOperation& rgo,
std::string_view employer_id,
fixed_point_t labor_offer
) {
return WorkforceEmployerRequest {
.employer_id = employer_id,
.labor_offer = labor_offer,
.requested = rgo.get_remaining_workforce_demand(),
.employer = &rgo,
.accepts = &rgo_accepts_worker_adapter,
.assign = &rgo_assign_worker_adapter
};
}

WorkforceEmployerRequest OpenVic::make_producer_workforce_request(
AggregateProducer& producer,
std::string_view employer_id,
fixed_point_t labor_offer
) {
producer.set_available_workforce(fixed_point_t::_0);

auto const& process = producer.get_production_type();

return WorkforceEmployerRequest {
.employer_id = employer_id,
.labor_offer = labor_offer,
.requested =
producer.get_capacity()
* fixed_point_t { type_safe::get(process.base_workforce_size) },
.employer = &producer,
.accepts = &producer_accepts_worker_adapter,
.assign = &producer_assign_worker_adapter
};
}