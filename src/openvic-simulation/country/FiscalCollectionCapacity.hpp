#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/reactive/DerivedState.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

namespace OpenVic {

class FiscalCollectionCapacity final {
private:
	MutableState<fixed_point_t> tax_base_coverage { fixed_point_t::_1 };
	MutableState<fixed_point_t> compliance_rate { fixed_point_t::_1 };
	MutableState<fixed_point_t> collection_execution { fixed_point_t::_1 };

	DerivedState<fixed_point_t> realization_factor {
		[this](DependencyTracker& tracker) -> fixed_point_t {
			return std::clamp(
				tax_base_coverage.get(tracker)
					* compliance_rate.get(tracker)
					* collection_execution.get(tracker),
				fixed_point_t::_0,
				fixed_point_t::_1
			);
		}
	};

	[[nodiscard]] static fixed_point_t clamp_unit(fixed_point_t value) {
		return std::clamp(value, fixed_point_t::_0, fixed_point_t::_1);
	}

public:
	FiscalCollectionCapacity() = default;
	FiscalCollectionCapacity(FiscalCollectionCapacity&&) = delete;
	FiscalCollectionCapacity(FiscalCollectionCapacity const&) = delete;
	FiscalCollectionCapacity& operator=(FiscalCollectionCapacity&&) = delete;
	FiscalCollectionCapacity& operator=(FiscalCollectionCapacity const&) = delete;

	void set_tax_base_coverage(fixed_point_t value) {
		tax_base_coverage.set(clamp_unit(value));
	}

	void set_compliance_rate(fixed_point_t value) {
		compliance_rate.set(clamp_unit(value));
	}

	void set_collection_execution(fixed_point_t value) {
		collection_execution.set(clamp_unit(value));
	}

	[[nodiscard]] fixed_point_t get_tax_base_coverage() const {
		return tax_base_coverage.get_untracked();
	}

	[[nodiscard]] fixed_point_t get_compliance_rate() const {
		return compliance_rate.get_untracked();
	}

	[[nodiscard]] fixed_point_t get_collection_execution() const {
		return collection_execution.get_untracked();
	}

	[[nodiscard]] fixed_point_t get_realization_factor() {
		return realization_factor.get_untracked();
	}

	[[nodiscard]] fixed_point_t get_realization_factor(DependencyTracker& tracker) {
		return realization_factor.get(tracker);
	}
};

}