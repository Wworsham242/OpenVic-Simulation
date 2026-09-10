#include "openvic-simulation/population/ProvinceDemographicAgeSexTsv.hpp"

#include <string>
#include <utility>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
std::string make_valid_004a12_row() {
std::string row =
"1"
"\tofficial_estimate"
"\tdirect_province"
"\t2025-07-01"
"\t2026-06-25"
"\t20"
"\tcensus-v2025";

for (
size_t cell = 0;
cell < DEMOGRAPHIC_AGE_SEX_CELL_COUNT;
++cell
) {
row += '\t';
row += std::to_string(
cell + 1
);
}

return row;
}
}

TEST_CASE(
"004A12 canonical TSV parses complete demographic import record",
"[convergence][004a12][population][demography][tsv]"
) {
const auto result =
parse_province_demographic_age_sex_tsv_row(
make_valid_004a12_row()
);

REQUIRE(result.parsed());

CHECK(
type_safe::get(
result.record.province_index
)
== 1
);

CHECK(
result.record.input.source_kind
== demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE
);

CHECK(
result.record.input.geography_basis
== demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE
);

CHECK(
result.record.input.reference_date
== Date { 2025, 7, 1 }
);

CHECK(
result.record.input.revision_date
== Date { 2026, 6, 25 }
);

CHECK(
result.record.input.authority_priority
== 20
);

CHECK(
result.record.input.source_identifier
== "census-v2025"
);

CHECK(
result.record.input.profile
.cohorts[0].female
== 1
);

CHECK(
result.record.input.profile
.cohorts[0].male
== 2
);

CHECK(
result.record.input.profile
.cohorts[
DEMOGRAPHIC_AGE_BAND_COUNT - 1
].male
== 36
);
}

TEST_CASE(
"004A12 canonical TSV rejects wrong field count",
"[convergence][004a12][population][demography][tsv][validation]"
) {
const auto result =
parse_province_demographic_age_sex_tsv_row(
"1\tofficial_estimate"
);

CHECK_FALSE(result.parsed());

CHECK(
result.status
== province_demographic_tsv_parse_status_t::
WRONG_FIELD_COUNT
);
}

TEST_CASE(
"004A12 canonical TSV rejects unknown source kind",
"[convergence][004a12][population][demography][tsv][validation]"
) {
std::string row =
make_valid_004a12_row();

const std::string old_value =
"official_estimate";

const auto position =
row.find(old_value);

REQUIRE(position != std::string::npos);

row.replace(
position,
old_value.size(),
"magic_source"
);

const auto result =
parse_province_demographic_age_sex_tsv_row(
row
);

CHECK_FALSE(result.parsed());

CHECK(
result.status
== province_demographic_tsv_parse_status_t::
INVALID_SOURCE_KIND
);
}

TEST_CASE(
"004A12 canonical TSV rejects invalid reference date rather than clamping",
"[convergence][004a12][population][demography][tsv][date]"
) {
std::string row =
make_valid_004a12_row();

const auto position =
row.find("2025-07-01");

REQUIRE(position != std::string::npos);

row.replace(
position,
10,
"2025-02-31"
);

const auto result =
parse_province_demographic_age_sex_tsv_row(
row
);

CHECK_FALSE(result.parsed());

CHECK(
result.status
== province_demographic_tsv_parse_status_t::
INVALID_REFERENCE_DATE
);
}

TEST_CASE(
"004A12 canonical TSV rejects empty source identifier",
"[convergence][004a12][population][demography][tsv][provenance]"
) {
std::string row =
make_valid_004a12_row();

const auto position =
row.find("census-v2025");

REQUIRE(position != std::string::npos);

row.erase(
position,
std::string {
"census-v2025"
}.size()
);

const auto result =
parse_province_demographic_age_sex_tsv_row(
row
);

CHECK_FALSE(result.parsed());

CHECK(
result.status
== province_demographic_tsv_parse_status_t::
EMPTY_SOURCE_IDENTIFIER
);
}

TEST_CASE(
"004A12 canonical TSV rejects overflowing weight",
"[convergence][004a12][population][demography][tsv][weight]"
) {
std::string row =
make_valid_004a12_row();

const size_t final_tab =
row.rfind('\t');

REQUIRE(
final_tab
!= std::string::npos
);

row.replace(
final_tab + 1,
std::string::npos,
"4294967296"
);

const auto result =
parse_province_demographic_age_sex_tsv_row(
row
);

CHECK_FALSE(result.parsed());

CHECK(
result.status
== province_demographic_tsv_parse_status_t::
INVALID_WEIGHT
);
}

TEST_CASE(
"004A12 parsed record feeds 004A11 registry unchanged",
"[convergence][004a12][population][demography][tsv][integration]"
) {
auto parsed =
parse_province_demographic_age_sex_tsv_row(
make_valid_004a12_row()
);

REQUIRE(parsed.parsed());

ProvinceDemographicAgeSexRegistry registry { 2 };

REQUIRE(
registry.add_import_record(
std::move(parsed.record)
)
== province_demographic_registry_add_status_t::
ADDED
);

registry.lock();

const auto resolved =
registry.resolve(
province_index_t { 1 },
Date { 2025, 7, 1 }
);

REQUIRE(resolved.has_selection());
REQUIRE(resolved.selected != nullptr);

CHECK(
resolved.selected->source_identifier
== "census-v2025"
);

CHECK(
resolved.selected->profile
.get_total_weight()
== 666
);
}
