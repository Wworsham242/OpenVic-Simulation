#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

struct SharedTransportRequest final {
	std::string flow_id;
	fixed_point_t requested = 0;
};

struct SharedTransportAllocation final {
	std::string flow_id;
	fixed_point_t requested = 0;
	fixed_point_t allocated = 0;
};

/// Coarse shared-capacity allocator for a strategic logistics segment.
///
/// If total requested flow exceeds segment capacity, capacity is distributed
/// proportionally to requested volume. Policy/priority mechanisms may replace
/// or pre-shape requests later; this object only enforces the physical shared
/// capacity constraint.
class SharedTransportCapacity final {
private:
	std::string capacity_id;
	fixed_point_t nominal_capacity = 0;
	fixed_point_t availability_fraction = fixed_point_t::_1;
	bool open = true;

public:
	SharedTransportCapacity(
		std::string new_capacity_id,
		fixed_point_t new_nominal_capacity
	) : capacity_id { std::move(new_capacity_id) },
		nominal_capacity { new_nominal_capacity } {}

	[[nodiscard]] bool is_valid() const {
		return !capacity_id.empty()
			&& nominal_capacity >= fixed_point_t::_0
			&& availability_fraction >= fixed_point_t::_0
			&& availability_fraction <= fixed_point_t::_1;
	}

	[[nodiscard]] fixed_point_t effective_capacity() const {
		if (!open || nominal_capacity <= fixed_point_t::_0) {
			return fixed_point_t::_0;
		}
		return nominal_capacity * availability_fraction;
	}

	[[nodiscard]] bool set_availability_fraction(fixed_point_t value) {
		if (value < fixed_point_t::_0 || value > fixed_point_t::_1) {
			return false;
		}
		availability_fraction = value;
		return true;
	}

	void set_open(bool value) {
		open = value;
	}

	[[nodiscard]] std::string_view get_capacity_id() const {
		return capacity_id;
	}

	[[nodiscard]] std::vector<SharedTransportAllocation> allocate(
		std::vector<SharedTransportRequest> const& requests
	) const {
		std::vector<SharedTransportAllocation> result;
		result.reserve(requests.size());

		fixed_point_t total_requested = 0;
		for (SharedTransportRequest const& request : requests) {
			if (request.requested > fixed_point_t::_0) {
				total_requested += request.requested;
			}
		}

		fixed_point_t const capacity = effective_capacity();

		for (SharedTransportRequest const& request : requests) {
			fixed_point_t allocated = 0;

			if (
				request.requested > fixed_point_t::_0 &&
				total_requested > fixed_point_t::_0 &&
				capacity > fixed_point_t::_0
			) {
				if (total_requested <= capacity) {
					allocated = request.requested;
				} else {
					allocated =
						capacity * request.requested / total_requested;
				}
			}

			result.push_back(SharedTransportAllocation {
				.flow_id = request.flow_id,
				.requested = request.requested,
				.allocated = allocated
			});
		}

		return result;
	}
};

}