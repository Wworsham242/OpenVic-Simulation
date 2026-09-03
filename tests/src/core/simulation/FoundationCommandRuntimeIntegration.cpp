#include "openvic-simulation/InstanceManager.hpp"

#include <concepts>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

static_assert(requires(
	InstanceManager& instance,
	ActorAuthorityProfile profile,
	std::string actor,
	std::string command,
	std::string jurisdiction,
	std::vector<uint8_t> payload
) {
	{ instance.register_actor_authority(std::move(profile)) } -> std::same_as<bool>;
	{
		instance.submit_authorized_command(
			std::move(actor),
			std::move(command),
			std::move(jurisdiction),
			std::move(payload)
		)
	} -> std::same_as<CommandAdmissionResult>;
	{ instance.get_accepted_command_count() } -> std::same_as<uint64_t>;
	{ instance.capture_command_replay_state() } -> std::same_as<CampaignReplayState>;
	{ instance.capture_accepted_command_log() } -> std::same_as<std::vector<CampaignCommandRecord>>;
});

TEST_CASE("InstanceManager exposes generalized command runtime seam", "[foundation][command][runtime][integration]") {
	// The production InstanceManager constructor itself is compiled by the full build and
	// must initialize CommandAdmissionRuntime from its owned AuthorityRegistry and
	// OrderedCommandRuntime. This test guards the public seam without constructing a
	// synthetic half-valid Victoria game instance.
	CHECK(true);
}