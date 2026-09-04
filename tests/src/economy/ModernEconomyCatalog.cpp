#include "openvic-simulation/economy/EconomyManager.hpp"

#include <filesystem>
#include <fstream>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
    ovdl::v2script::Parser parse_text_file(
        std::filesystem::path const& path,
        std::string_view text
    ) {
        std::ofstream out { path };
        out << text;
        out.close();
        return Dataloader::parse_defines(path);
    }
}

TEST_CASE(
    "Modern goods preload survives legacy goods finalisation",
    "[economy][modern-catalog][goods]"
) {
    namespace fs = std::filesystem;
    EconomyManager economy;

    const fs::path dir = fs::temp_directory_path() / "openvic-live-economy-005-goods";
    fs::remove_all(dir);
    fs::create_directories(dir);

    auto modern_parser = parse_text_file(
        dir / "modern_goods.txt",
        R"(
modern_industrial = {
    modern_iron_ore = { color = { 80 80 80 } cost = 1 }
    modern_primary_steel = { color = { 100 110 120 } cost = 2 }
    modern_industrial_machinery = { color = { 140 150 160 } cost = 4 }
}
)"
    );
    REQUIRE(economy.load_modern_goods_catalog_file(modern_parser.get_file_node()));

    auto base_parser = parse_text_file(
        dir / "goods.txt",
        R"(
legacy_category = {
    legacy_good = { color = { 10 20 30 } cost = 3 }
}
)"
    );
    REQUIRE(economy.get_good_definition_manager().load_goods_file(base_parser.get_file_node()));

    GoodDefinitionManager const& goods = economy.get_good_definition_manager();
    CHECK(goods.get_good_definition_by_identifier("modern_iron_ore") != nullptr);
    CHECK(goods.get_good_definition_by_identifier("modern_primary_steel") != nullptr);
    CHECK(goods.get_good_definition_by_identifier("modern_industrial_machinery") != nullptr);
    CHECK(goods.get_good_definition_by_identifier("legacy_good") != nullptr);

    fs::remove_all(dir);
}

TEST_CASE(
    "Modern aggregate processes have no Victoria actor semantics",
    "[economy][modern-catalog][production]"
) {
    namespace fs = std::filesystem;
    EconomyManager economy;
    GameRulesManager rules;
    PopManager pops;

    GoodDefinitionManager& goods = economy.get_good_definition_manager();
    REQUIRE(goods.add_good_category("modern_industrial", 3));
    GoodCategory const* category_const = goods.get_good_category_by_identifier("modern_industrial");
    REQUIRE(category_const != nullptr);
    GoodCategory& category = const_cast<GoodCategory&>(*category_const);

    REQUIRE(goods.add_good_definition(
        "modern_iron_ore", colour_rgb_t {}, category,
        fixed_point_t::_1, true, true, false, false
    ));
    REQUIRE(goods.add_good_definition(
        "modern_primary_steel", colour_rgb_t {}, category,
        fixed_point_t(2), true, true, false, false
    ));
    REQUIRE(goods.add_good_definition(
        "modern_industrial_machinery", colour_rgb_t {}, category,
        fixed_point_t(4), true, true, false, false
    ));

    const fs::path dir = fs::temp_directory_path() / "openvic-live-economy-005-production";
    fs::remove_all(dir);
    fs::create_directories(dir);

    auto parser = parse_text_file(
        dir / "modern_production_types.txt",
        R"(
modern_primary_steel_process = {
    workforce = 1
    input_goods = { modern_iron_ore = 1 }
    output_good = modern_primary_steel
    value = 1
}
modern_industrial_machinery_process = {
    workforce = 1
    input_goods = { modern_primary_steel = 2 }
    output_good = modern_industrial_machinery
    value = 1
}
)"
    );

    REQUIRE(economy.load_modern_production_catalog_file(rules, pops, parser.get_file_node()));

    ProductionType const* steel = economy.get_production_type_manager()
        .get_production_type_by_identifier("modern_primary_steel_process");
    ProductionType const* machinery = economy.get_production_type_manager()
        .get_production_type_by_identifier("modern_industrial_machinery_process");

    REQUIRE(steel != nullptr);
    REQUIRE(machinery != nullptr);
    CHECK(steel->template_type == ProductionType::template_type_t::AGGREGATE);
    CHECK(machinery->template_type == ProductionType::template_type_t::AGGREGATE);
    CHECK(!steel->owner.has_value());
    CHECK(steel->get_jobs().empty());
    CHECK(steel->output_good.get_identifier() == "modern_primary_steel");
    CHECK(machinery->output_good.get_identifier() == "modern_industrial_machinery");

    fs::remove_all(dir);
}