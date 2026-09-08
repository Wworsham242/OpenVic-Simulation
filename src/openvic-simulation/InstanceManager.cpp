#include "InstanceManager.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/misc/GameAction.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

InstanceManager::InstanceManager(
	GameRulesManager const& new_game_rules_manager,
	DefinitionManager const& new_definition_manager,
	gamestate_updated_func_t gamestate_updated_callback
) : thread_pool { today },
	definition_manager { new_definition_manager },
	game_action_manager { *this },
	game_rules_manager { new_game_rules_manager },
	good_instance_manager {
		new_definition_manager.get_economy_manager().get_good_definition_manager(),
		new_game_rules_manager
	},
	market_instance {
		thread_pool,
		new_definition_manager.get_define_manager().get_country_defines(),
		good_instance_manager
	},
	artisanal_producer_deps {
		new_definition_manager.get_define_manager().get_economy_defines(),
		new_definition_manager.get_economy_manager().get_good_definition_manager().get_good_definitions(),
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache()
	},
	country_instance_deps {
		new_definition_manager.get_economy_manager().get_building_type_manager().get_building_types(),
		new_definition_manager.get_define_manager().get_country_defines(),
		country_relation_manager,
		new_definition_manager.get_crime_manager().get_crime_modifiers(),
		new_definition_manager.get_define_manager().get_end_date(),
		new_definition_manager.get_define_manager().get_diplomacy_defines(),
		new_definition_manager.get_define_manager().get_economy_defines(),
		new_definition_manager.get_politics_manager().get_ideology_manager().get_ideologies(),
		new_definition_manager.get_research_manager().get_invention_manager().get_inventions(),
		new_game_rules_manager,
		good_instance_manager.get_good_instances(),
		good_instance_manager,
		new_definition_manager.get_politics_manager().get_government_type_manager().get_government_types(),
		market_instance,
		new_definition_manager.get_define_manager().get_military_defines(),
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		PopsAggregateDeps{
			ideology_index_t(
				new_definition_manager.get_politics_manager().get_ideology_manager().get_ideology_count()
			),
			party_policy_index_t(
				new_definition_manager.get_politics_manager().get_issue_manager().get_party_policy_count()
			),
			pop_type_index_t(
				new_definition_manager.get_pop_manager().get_pop_type_count()
			),
			reform_index_t(
				new_definition_manager.get_politics_manager().get_issue_manager().get_reform_count()
			),
			strata_index_t(
				new_definition_manager.get_pop_manager().get_strata_count()
			)
		},
		new_definition_manager.get_pop_manager().get_pop_types(),
		new_definition_manager.get_politics_manager().get_issue_manager().get_reform_groups(),
		new_definition_manager.get_military_manager().get_unit_type_manager().get_regiment_types(),
		new_definition_manager.get_military_manager().get_unit_type_manager().get_ship_types(),
		new_definition_manager.get_pop_manager().get_stratas(),
		new_definition_manager.get_research_manager().get_technology_manager().get_technologies(),
		new_definition_manager.get_military_manager().get_unit_type_manager()
	},
	pops_aggregate_deps {
			ideology_index_t(
				new_definition_manager.get_politics_manager().get_ideology_manager().get_ideology_count()
			),
			party_policy_index_t(
				new_definition_manager.get_politics_manager().get_issue_manager().get_party_policy_count()
			),
			pop_type_index_t(
				new_definition_manager.get_pop_manager().get_pop_type_count()
			),
			reform_index_t(
				new_definition_manager.get_politics_manager().get_issue_manager().get_reform_count()
			),
			strata_index_t(
				new_definition_manager.get_pop_manager().get_strata_count()
			)
		},
	pop_deps {
		artisanal_producer_deps,
		market_instance,
		pops_aggregate_deps
	},
	rgo_deps {
		market_instance,
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		pop_type_index_t(new_definition_manager.get_pop_manager().get_pop_type_count())
	},
	province_instance_deps {
		new_definition_manager.get_economy_manager().get_building_type_manager(),
		new_game_rules_manager,
		pops_aggregate_deps,
		rgo_deps,
		new_definition_manager.get_pop_manager().get_stratas()
	},
	global_flags { "global" },
	country_instance_manager {
		new_definition_manager.get_define_manager().get_country_defines(),
		new_definition_manager.get_country_definition_manager(),
		country_instance_deps,
		good_instance_manager,
		new_definition_manager.get_define_manager().get_pops_defines(),
		new_definition_manager.get_pop_manager().get_pop_types(),
		new_definition_manager.get_military_manager().get_unit_type_manager().get_regiment_types(),
		thread_pool
	},
	unit_instance_manager {
		new_definition_manager.get_pop_manager().get_culture_manager(),
		new_definition_manager.get_military_manager().get_leader_trait_manager(),
		new_definition_manager.get_define_manager().get_military_defines()
	},
	politics_instance_manager {
		*this,
		new_definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()
	},
	map_instance {
		new_definition_manager.get_map_definition(),
		province_instance_deps,
		thread_pool
	},
	simulation_clock {
		[this]() -> void {
			queue_game_action<tick_argument_t>();
		},
		[this]() -> void {
			execute_game_actions();
			update_gamestate();
		}
	},
	command_admission_runtime { authority_registry, ordered_command_runtime },
	console_instance { *this },
	gamestate_updated { gamestate_updated_callback ? std::move(gamestate_updated_callback) : []() {} } {}

void InstanceManager::set_gamestate_needs_update() {
	if (!currently_updating_gamestate) {
		gamestate_needs_update = true;
	} else {
		spdlog::error_s("Attempted to queue a gamestate update already updating the gamestate!");
	}
}

void InstanceManager::update_gamestate() {
	if (currently_updating_gamestate) {
		spdlog::error_s("Attempted to update gamestate while already updating gamestate!");
		return;
	}

	if (!gamestate_needs_update) {
		return;
	}

	currently_updating_gamestate = true;

	SPDLOG_INFO("Update: {}", today);

	update_modifier_sums();

	// Update gamestate...
	map_instance.update_gamestate(*this);
	country_instance_manager.update_gamestate(today, map_instance);
	unit_instance_manager.update_gamestate();

	gamestate_updated();
	gamestate_needs_update = false;

	currently_updating_gamestate = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void InstanceManager::tick() {

	// The legacy day remains a compatibility driver. The live economy consumes
	// crossed SimTime cadence boundaries, independently of the Date calendar.
	static constexpr int64_t LEGACY_DAY_SIMULATION_TICKS = 24;
	SimTime const previous_time = simulation_timeline.current_time();
	if (!simulation_timeline.advance(LEGACY_DAY_SIMULATION_TICKS)) {
		spdlog::error_s("Simulation timeline could not advance; refusing legacy daily tick.");
		return;
	}

	today++;

	SPDLOG_INFO("Tick: {}", today);

	// Tick...
	country_instance_manager.country_manager_tick_before_map();

// B3 phase 1: all POP employment resets and RGO demand preparation
// finish before any employer receives workers.
map_instance.prepare_employment_phase();

// One authoritative market clear remains shared by legacy and modern orders.
auto clear_market = [this]() { market_instance.execute_orders(); };

if (live_economy_runtime != nullptr) {
live_economy_runtime->run_due_daily_cycles(
previous_time,
simulation_timeline.current_time(),
[this](SimTime) -> std::optional<WorkforcePool> {
auto workforce =
live_economy_runtime->prepare_upstream_site(map_instance);

ProductiveSiteBinding const* binding =
live_economy_runtime->get_upstream_site_binding();

if (!workforce.has_value() || binding == nullptr) {
map_instance.allocate_legacy_rgo_workforce();
map_instance.finish_rgo_production();
return std::nullopt;
}

ProvinceInstance* province =
map_instance.get_province_instance_by_identifier(
binding->province_id
);

if (province == nullptr) {
map_instance.allocate_legacy_rgo_workforce();
map_instance.finish_rgo_production();
return std::nullopt;
}

// Non-contested provinces keep inherited RGO allocation.
map_instance.allocate_legacy_rgo_workforce_except(
binding->province_id
);

AggregateProducer& producer =
live_economy_runtime
->get_upstream_producer_for_workforce_allocation();

std::string const rgo_employer_id =
"rgo:" + binding->province_id;

std::string const producer_employer_id =
"site:" + binding->province_id + ":" +
binding->building_id;

WorkforceEmployerRequest rgo_request =
make_rgo_workforce_request(
province->get_mutable_rgo(),
rgo_employer_id,
fixed_point_t::_1
);

WorkforceEmployerRequest producer_request =
make_producer_workforce_request(
producer,
producer_employer_id,
fixed_point_t::_1
);

fixed_point_t const producer_requested =
producer_request.requested;

auto allocations = allocate_competing_employers(
{
rgo_request,
producer_request
},
*workforce
);

fixed_point_t producer_allocated = fixed_point_t::_0;

for (
WorkforceEmployerAllocation const& allocation :
allocations
) {
if (
allocation.employer_id ==
producer_employer_id
) {
producer_allocated = allocation.allocated;
break;
}
}

live_economy_runtime
->set_preallocated_upstream_workforce(
WorkforceAllocationResult {
.requested = producer_requested,
.allocated = producer_allocated
}
);

// RGO output now consumes the assignment produced by the
// same labor authority as the modern productive site.
map_instance.finish_rgo_production();

// No POP pool is returned because the modern producer has
// already received its authoritative assignment.
return std::nullopt;
},
clear_market
);
} else {
map_instance.allocate_legacy_rgo_workforce();
map_instance.finish_rgo_production();
clear_market();
}
country_instance_manager.country_manager_tick_after_map();
	unit_instance_manager.tick();

	if (today.is_month_start()) {
		market_instance.record_price_history();
	}
}

void InstanceManager::execute_game_actions() {
	if (currently_executing_game_actions) {
		spdlog::error_s("Attempted to execute game actions while already executing game actions!");
		return;
	}

	if (game_action_queue.empty()) {
		return;
	}

	currently_executing_game_actions = true;

	// Temporary gamestate/UI update trigger, to be replaced with a more sophisticated targeted system
	bool needs_gamestate_update = false;

	for (game_action_t const& game_action : game_action_queue) {
		needs_gamestate_update |= game_action_manager.execute_game_action(game_action);
	}

	game_action_queue.clear();

	if (needs_gamestate_update) {
		set_gamestate_needs_update();
	}

	currently_executing_game_actions = false;
}

bool InstanceManager::setup() {
	if (is_game_instance_setup()) {
		spdlog::error_s("Cannot setup game instance - already set up!");
		return false;
	}

	thread_pool.initialise_threadpool(
		game_rules_manager,
		good_instance_manager,
		definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		definition_manager.get_define_manager().get_pops_defines(),
		definition_manager.get_economy_manager().get_production_type_manager(),
		strata_index_t(definition_manager.get_pop_manager().get_strata_count()),
		good_instance_manager.get_good_instances(),
		country_instance_manager.get_country_instances(),
		map_instance.get_province_instances()
	);

	// LIVE-ECONOMY-002: instantiate only from explicit scenario-owned
	// definitions. The runtime no longer invents goods, processes, nodes,
	// producer capacities, source inflow, or transport legs.
	LiveEconomyScenarioDefinition const* const live_scenario =
		definition_manager.get_economy_manager().get_live_economy_scenario();

	if (live_scenario != nullptr && live_scenario->is_valid()) {
		live_economy_runtime = std::make_unique<LiveEconomyRuntime>(
			game_rules_manager,
			good_instance_manager,
			*live_scenario
		);

		SPDLOG_INFO(
			"LIVE-ECONOMY-002 configured from scenario: upstream={}, downstream={}",
			live_scenario->upstream_process->get_identifier(),
			live_scenario->downstream_process->get_identifier()
		);
	} else {
		SPDLOG_INFO(
			"LIVE-ECONOMY-002 has no configured scenario; live aggregate economy remains disabled."
		);
	}
	game_instance_setup = true;

	return true;
}

bool InstanceManager::load_bookmark(Bookmark const& new_bookmark) {
	if (is_bookmark_loaded()) {
		spdlog::error_s("Cannot load bookmark - already loaded!");
		return false;
	}

	if (!is_game_instance_setup()) {
		spdlog::error_s("Cannot load bookmark - game instance not set up!");
		return false;
	}

	bookmark = &new_bookmark;

	SPDLOG_INFO("Loading bookmark {} with start date {}", bookmark->get_name(), bookmark->date);

	if (!definition_manager.get_define_manager().in_game_period(bookmark->date)) {
		spdlog::warn_s("Bookmark date {} is not in the game's time period!", bookmark->date);
	}

	today = bookmark->date;

	politics_instance_manager.setup_starting_ideologies();
	bool ret = map_instance.apply_history_to_provinces(
		definition_manager.get_history_manager().get_province_manager(), today,
		country_instance_manager,
		definition_manager.get_define_manager().get_military_defines(),
		pop_deps,
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_issue_manager().get_reforms()
	);

	// It is important that province history is applied before country history as province history includes
	// generating pops which then have stats like literacy and consciousness set by country history.

	ret &= country_instance_manager.apply_history_to_countries(*this);

	ret &= map_instance.get_state_manager().generate_states(
		definition_manager.get_map_definition(),
		map_instance,
		pops_aggregate_deps,
		definition_manager.get_pop_manager().get_stratas(),
		definition_manager.get_pop_manager().get_pop_types()
	);

	bool all_has_state = true;
	for (ProvinceInstance const& province_instance : map_instance.get_province_instances()) {
		if (!province_instance.province_definition.is_water() && OV_unlikely(province_instance.get_state() == nullptr)) {
			all_has_state = false;
			spdlog::error_s("Province {} has no state.", province_instance);
			continue;
		}
	}
	OV_ERR_FAIL_COND_V_MSG(!all_has_state, false, "At least one land province has no state");

	update_modifier_sums();
	map_instance.initialise_for_new_game(*this);
	country_instance_manager.update_gamestate(today, map_instance);
	market_instance.execute_orders();

	return ret;
}

bool InstanceManager::start_game_session() {
	if (is_game_session_started()) {
		spdlog::error_s("Cannot start game session - already started!");
		return false;
	}

	session_start = time(nullptr);
	simulation_clock.reset();
	set_gamestate_needs_update();

	game_session_started = true;

	return true;
}

bool InstanceManager::update_clock() {
	if (!is_game_session_started()) {
		spdlog::error_s("Cannot update clock - game session not started!");
		return false;
	}

	simulation_clock.conditionally_advance_game();
	return true;
}

void InstanceManager::force_tick_and_update() {
	simulation_clock.force_advance_game();
}

bool InstanceManager::set_today_and_update(Date new_today) {
	if (!is_game_session_started()) {
		spdlog::error_s("Cannot update clock - game session not started!");
		return false;
	}

	today = new_today;
	gamestate_needs_update = true;
	update_gamestate();
	return true;
}

void InstanceManager::update_modifier_sums() {
	// Calculate national country modifier sums first, then local province modifier sums, adding province contributions
	// to controller countries' modifier sums if each province has a controller. This results in every country having a
	// full copy of all the modifiers affecting them in their modifier sum, but provinces only having their directly/locally
	// applied modifiers in their modifier sum, hence requiring owner country modifier effect values to be looked up when
	// determining the value of a global effect on the province.
	country_instance_manager.update_modifier_sums(
		today, definition_manager.get_modifier_manager().get_static_modifier_cache()
	);
	map_instance.update_modifier_sums(
		today, definition_manager.get_modifier_manager().get_static_modifier_cache()
	);
}

bool InstanceManager::queue_game_action(game_action_t&& game_action) {
	if (currently_executing_game_actions) {
		spdlog::error_s("Attempted to queue a game action while already executing game actions!");
		return false;
	}

	game_action_queue.emplace_back(std::move(game_action));
	return true;
}
CommandAdmissionResult InstanceManager::queue_authorized_legacy_mobilise(
	std::string actor_id,
	std::string jurisdiction_id,
	country_index_t country_index,
	bool new_is_mobilised
) {
	if (currently_executing_game_actions) {
		spdlog::error_s(
			"Attempted to submit authorized mobilise command while executing game actions."
		);
		return CommandAdmissionResult::invalid_request;
	}

	std::vector<uint8_t> const payload =
		LegacyMobiliseCommand::encode(country_index, new_is_mobilised);

	CommandAdmissionResult const admission = submit_authorized_command(
		std::move(actor_id),
		LegacyMobiliseCommand::COMMAND_TYPE,
		std::move(jurisdiction_id),
		payload
	);

	if (admission != CommandAdmissionResult::accepted) {
		return admission;
	}

	if (!queue_game_action<set_mobilise_argument_t>(country_index, new_is_mobilised)) {
		spdlog::error_s(
			"Authorized mobilise command was accepted but legacy GameAction queueing failed."
		);
		return CommandAdmissionResult::invalid_request;
	}

	return CommandAdmissionResult::accepted;
}
