#pragma once

#include <memory>
#include <utility>

#include <function2/function2.hpp>

#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/simulation/AuthorityRegistry.hpp"
#include "openvic-simulation/core/simulation/LegacyMobiliseCommand.hpp"
#include "openvic-simulation/core/simulation/LiveCommandTimelineSnapshot.hpp"
#include "openvic-simulation/core/simulation/PositionSessionBootstrap.hpp"
#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"
#include "openvic-simulation/country/CountryInstanceDeps.hpp"
#include "openvic-simulation/country/CountryInstanceManager.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/LiveEconomyRuntime.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerDeps.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperationDeps.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/map/ProvinceInstanceDeps.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/misc/GameAction.hpp"
#include "openvic-simulation/misc/SimulationClock.hpp"
#include "openvic-simulation/politics/PoliticsInstanceManager.hpp"
#include "openvic-simulation/player/PlayerManager.hpp"
#include "openvic-simulation/population/PopDeps.hpp"
#include "openvic-simulation/population/PopsAggregateDeps.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

namespace OpenVic {

	struct DefinitionManager;
	struct Bookmark;

	struct InstanceManager {
		friend GameActionManager;

		using gamestate_updated_func_t = fu2::function_base<true, true, fu2::capacity_can_hold<void*>, false, false, void()>;

	private:
		ThreadPool thread_pool;

		CountryRelationManager PROPERTY_REF(country_relation_manager);
		const GameActionManager game_action_manager;
		GameRulesManager const& game_rules_manager;
		GoodInstanceManager PROPERTY_REF(good_instance_manager);
		MarketInstance PROPERTY_REF(market_instance);
		std::unique_ptr<LiveEconomyRuntime> live_economy_runtime;

		ArtisanalProducerDeps artisanal_producer_deps;
		CountryInstanceDeps country_instance_deps;
		PopsAggregateDeps pops_aggregate_deps;
		PopDeps pop_deps;
		ResourceGatheringOperationDeps rgo_deps;
		ProvinceInstanceDeps province_instance_deps;

		FlagStrings PROPERTY_REF(global_flags);

		CountryInstanceManager PROPERTY_REF(country_instance_manager);
		UnitInstanceManager PROPERTY_REF(unit_instance_manager);
		PoliticsInstanceManager PROPERTY_REF(politics_instance_manager);
		/* Near the end so it is freed after other managers that may depend on it,
		 * e.g. if we want to remove military units from the province they're in when they're destructed. */
		MapInstance PROPERTY_REF(map_instance);
		SimulationClock PROPERTY_REF(simulation_clock);
		SimulationTimeline simulation_timeline;
		PlayerManager PROPERTY_REF(player_manager);

		// FOUNDATION-009: live generalized command-admission state.
		AuthorityRegistry authority_registry;
		OrderedCommandRuntime ordered_command_runtime;
		CommandAdmissionRuntime command_admission_runtime;
		ConsoleInstance PROPERTY_REF(console_instance);

		bool PROPERTY_CUSTOM_PREFIX(game_instance_setup, is, false);
		bool PROPERTY_CUSTOM_PREFIX(game_session_started, is, false);

		time_t session_start = 0; /* SS-54, as well as allowing time-tracking */
		Bookmark const* PROPERTY(bookmark, nullptr);
		Date PROPERTY(today);
		gamestate_updated_func_t gamestate_updated;
		memory::vector<game_action_t> game_action_queue;
		bool gamestate_needs_update = false, currently_updating_gamestate = false, currently_executing_game_actions = false;

		void update_modifier_sums();
		void set_gamestate_needs_update();
		void update_gamestate();
		void tick();

		void execute_game_actions();

	public:
		DefinitionManager const& definition_manager;

		InstanceManager(
			GameRulesManager const& new_game_rules_manager,
			DefinitionManager const& new_definition_manager,
			gamestate_updated_func_t gamestate_updated_callback
		);

		inline constexpr bool is_bookmark_loaded() const {
			return bookmark != nullptr;
		}

		bool setup();
		bool load_bookmark(Bookmark const& new_bookmark);
		bool start_game_session();
		bool update_clock();
		void force_tick_and_update();

		bool set_today_and_update(Date new_today);

		[[nodiscard]] SimTime get_simulation_time() const {
			return simulation_timeline.current_time();
		}

		[[nodiscard]] LiveEconomyStatus get_live_economy_status() const {
			return live_economy_runtime != nullptr
				? live_economy_runtime->get_status()
				: LiveEconomyStatus {};
		}
		/// Register one generalized actor authority profile.
		[[nodiscard]] bool register_actor_authority(ActorAuthorityProfile profile) {
			return authority_registry.register_profile(std::move(profile));
		}
		[[nodiscard]] bool bootstrap_position_occupancy(
			PositionOccupancy occupancy,
			std::vector<AuthorityGrant> grants
		) {
			if (is_game_session_started()) {
				return false;
			}
			return PositionSessionBootstrap::configure(
				player_manager,
				authority_registry,
				std::move(occupancy),
				std::move(grants)
			);
		}

		[[nodiscard]] CommandAdmissionResult submit_occupied_position_command(
			std::string command_type,
			std::vector<uint8_t> payload
		) {
			PositionOccupancy const* const occupancy = player_manager.get_position_occupancy();
			if (occupancy == nullptr) {
				return CommandAdmissionResult::invalid_request;
			}
			return submit_authorized_command(
				std::string { occupancy->authority_actor_id() },
				std::move(command_type),
				std::string { occupancy->jurisdiction_id() },
				std::move(payload)
			);
		}

		[[nodiscard]] CommandAdmissionResult queue_occupied_legacy_mobilise(
			country_index_t country_index,
			bool new_is_mobilised
		) {
			PositionOccupancy const* const occupancy = player_manager.get_position_occupancy();
			if (occupancy == nullptr) {
				return CommandAdmissionResult::invalid_request;
			}
			return queue_authorized_legacy_mobilise(
				std::string { occupancy->authority_actor_id() },
				std::string { occupancy->jurisdiction_id() },
				country_index,
				new_is_mobilised
			);
		}

		/// Submit through generalized authority using authoritative simulation time.
		///
		/// FOUNDATION-009 records authorized commands but does not yet execute domain mutation.
		[[nodiscard]] CommandAdmissionResult submit_authorized_command(
			std::string actor_id,
			std::string command_type,
			std::string jurisdiction_id,
			std::vector<uint8_t> payload
		) {
			return command_admission_runtime.submit(
				simulation_timeline.current_time(),
				std::move(actor_id),
				std::move(command_type),
				std::move(jurisdiction_id),
				std::move(payload)
			);
		}

		[[nodiscard]] uint64_t get_accepted_command_count() const {
			return ordered_command_runtime.accepted_command_count();
		}

		[[nodiscard]] CampaignReplayState capture_command_replay_state() const {
			return ordered_command_runtime.capture_replay_state();
		}

		[[nodiscard]] std::vector<CampaignCommandRecord> capture_accepted_command_log() const {
			return ordered_command_runtime.capture_command_log();
		}
		[[nodiscard]] LiveCommandTimelineSnapshot capture_live_command_timeline_state() const {
			return LiveCommandTimelineState::capture(simulation_timeline, ordered_command_runtime);
		}

		[[nodiscard]] bool restore_live_command_timeline_state(
			LiveCommandTimelineSnapshot const& snapshot
		) {
			return LiveCommandTimelineState::restore(
				snapshot,
				simulation_timeline,
				ordered_command_runtime
			);
		}
		template<typename T, typename... Args>
		bool queue_game_action(Args&&... args) {
			return queue_game_action(
				game_action_t(
					std::in_place_type<T>,
					std::forward<Args>(args)...
				)
			);
		}
		bool queue_game_action(game_action_t&& game_action);
		[[nodiscard]] CommandAdmissionResult queue_authorized_legacy_mobilise(
			std::string actor_id,
			std::string jurisdiction_id,
			country_index_t country_index,
			bool new_is_mobilised
		);
	};
}
