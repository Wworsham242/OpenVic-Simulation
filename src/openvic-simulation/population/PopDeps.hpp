#pragma once

namespace OpenVic {
	struct ArtisanalProducerDeps;
	struct MarketInstance;
	struct PopsAggregateDeps;

	struct PopDeps {
		ArtisanalProducerDeps const& artisanal_producer_deps;
		MarketInstance& market_instance;
		PopsAggregateDeps const& pops_aggregate_deps;

/*
 * Optional population capabilities.
 *
 * Default true preserves existing OpenVic behavior. Native/general
 * scenario assembly may explicitly disable this capability without
 * changing authoritative population size or employment.
 */
bool enable_nutrition_health_capability = true;
	};
}