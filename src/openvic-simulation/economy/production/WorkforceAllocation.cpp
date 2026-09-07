#include "WorkforceAllocation.hpp"

#include <algorithm>

#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopType.hpp"

using namespace OpenVic;

fixed_point_t OpenVic::allocate_producer_workforce(
	AggregateProducer& producer, std::span<Pop> pops, WorkforceAllocationResult* result
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
