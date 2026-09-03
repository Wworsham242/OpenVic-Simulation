#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/core/simulation/LegacyMobiliseCommand.hpp"

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
	std::string actor,
	std::string jurisdiction,
	country_index_t country,
	bool mobilised
) {
	{
		instance.queue_authorized_legacy_mobilise(
			std::move(actor),
			std::move(jurisdiction),
			country,
			mobilised
		)
	} -> std::same_as<CommandAdmissionResult>;
});

TEST_CASE("Legacy mobilise compatibility payload is deterministic", "[foundation][command][legacy][military]") {
	country_index_t const country { 0x78563412u };

	auto const a = LegacyMobiliseCommand::encode(country, true);
	auto const b = LegacyMobiliseCommand::encode(country, true);

	CHECK(a == b);
	REQUIRE(a.size() == LegacyMobiliseCommand::PAYLOAD_SIZE);
	CHECK(a[0] == 0x12u);
	CHECK(a[1] == 0x34u);
	CHECK(a[2] == 0x56u);
	CHECK(a[3] == 0x78u);
	CHECK(a[4] == 1u);
}

TEST_CASE("Legacy mobilise payload round trips", "[foundation][command][legacy][replay]") {
	country_index_t const original_country { 42u };
	bool const original_state = false;

	auto const payload = LegacyMobiliseCommand::encode(original_country, original_state);

	country_index_t decoded_country { 0u };
	bool decoded_state = true;

	REQUIRE(LegacyMobiliseCommand::decode(payload, decoded_country, decoded_state));
	CHECK(type_safe::get(decoded_country) == type_safe::get(original_country));
	CHECK(decoded_state == original_state);
}

TEST_CASE("Legacy mobilise decoder rejects malformed payload", "[foundation][command][legacy][validation]") {
	country_index_t country { 0u };
	bool state = false;

	CHECK_FALSE(LegacyMobiliseCommand::decode({}, country, state));
	CHECK_FALSE(LegacyMobiliseCommand::decode({ 0, 0, 0, 0, 2 }, country, state));
}

TEST_CASE("Mobilise command type is stable", "[foundation][command][legacy][identity]") {
	CHECK(std::string { LegacyMobiliseCommand::COMMAND_TYPE } == "military.set_mobilised");
	CHECK(LegacyMobiliseCommand::PAYLOAD_SCHEMA_VERSION == 1);
}