#pragma once

#include <optional>

#include "openvic-simulation/country/FiscalCollectionCapacity.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/// Optional scenario/history values for the fiscal collection mechanism.
///
/// Missing values are intentionally no-ops so dated history entries can override one
/// dimension without resetting the other dimensions.
struct FiscalCollectionHistory final {
	std::optional<fixed_point_t> tax_base_coverage;
	std::optional<fixed_point_t> compliance_rate;
	std::optional<fixed_point_t> collection_execution;

	[[nodiscard]] bool empty() const {
		return !tax_base_coverage.has_value()
			&& !compliance_rate.has_value()
			&& !collection_execution.has_value();
	}

	void apply_to(FiscalCollectionCapacity& capacity) const {
		if (tax_base_coverage.has_value()) {
			capacity.set_tax_base_coverage(*tax_base_coverage);
		}
		if (compliance_rate.has_value()) {
			capacity.set_compliance_rate(*compliance_rate);
		}
		if (collection_execution.has_value()) {
			capacity.set_collection_execution(*collection_execution);
		}
	}
};

}