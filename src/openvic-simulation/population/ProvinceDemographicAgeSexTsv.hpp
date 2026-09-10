#pragma once

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include "openvic-simulation/population/ProvinceDemographicAgeSexRegistry.hpp"

namespace OpenVic {
inline constexpr size_t
PROVINCE_DEMOGRAPHIC_TSV_METADATA_FIELD_COUNT = 7;

inline constexpr size_t
PROVINCE_DEMOGRAPHIC_TSV_FIELD_COUNT =
PROVINCE_DEMOGRAPHIC_TSV_METADATA_FIELD_COUNT
+ DEMOGRAPHIC_AGE_SEX_CELL_COUNT;

enum class province_demographic_tsv_parse_status_t : uint8_t {
PARSED = 0,
WRONG_FIELD_COUNT,
INVALID_PROVINCE_INDEX,
INVALID_SOURCE_KIND,
INVALID_GEOGRAPHY_BASIS,
INVALID_REFERENCE_DATE,
INVALID_REVISION_DATE,
INVALID_AUTHORITY_PRIORITY,
EMPTY_SOURCE_IDENTIFIER,
INVALID_WEIGHT,
EMPTY_PROFILE
};

struct ProvinceDemographicAgeSexTsvParseResult {
ProvinceDemographicAgeSexImportRecord record {};

province_demographic_tsv_parse_status_t status =
province_demographic_tsv_parse_status_t::
WRONG_FIELD_COUNT;

[[nodiscard]]
bool parsed() const {
return
status
== province_demographic_tsv_parse_status_t::
PARSED;
}
};

template<typename Integer>
[[nodiscard]]
inline bool parse_demographic_unsigned_integer(
std::string_view text,
Integer& value
) {
if (text.empty()) {
return false;
}

Integer parsed {};

auto const result =
std::from_chars(
text.data(),
text.data() + text.size(),
parsed
);

if (
result.ec != std::errc {}
|| result.ptr
!= text.data() + text.size()
) {
return false;
}

value = parsed;
return true;
}

[[nodiscard]]
inline bool parse_demographic_iso_date(
std::string_view text,
Date& value
) {
if (
text.size() != 10
|| text[4] != '-'
|| text[7] != '-'
) {
return false;
}

uint16_t year = 0;
uint8_t month = 0;
uint8_t day = 0;

if (
!parse_demographic_unsigned_integer(
text.substr(0, 4),
year
)
|| !parse_demographic_unsigned_integer(
text.substr(5, 2),
month
)
|| !parse_demographic_unsigned_integer(
text.substr(8, 2),
day
)
) {
return false;
}

if (
year
> static_cast<uint16_t>(
std::numeric_limits<
Date::year_t
>::max()
)
|| month < 1
|| month > Date::MONTHS_IN_YEAR
) {
return false;
}

const auto month_index =
static_cast<size_t>(month - 1);

if (
day < 1
|| day > Date::DAYS_IN_MONTH[
month_index
]
) {
return false;
}

value =
Date {
static_cast<Date::year_t>(year),
month,
day
};

return true;
}

[[nodiscard]]
inline bool parse_demographic_source_kind(
std::string_view text,
demographic_age_sex_source_kind_t& value
) {
if (text == "observed_enumeration") {
value =
demographic_age_sex_source_kind_t::
OBSERVED_ENUMERATION;
} else if (text == "official_estimate") {
value =
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE;
} else if (text == "projection") {
value =
demographic_age_sex_source_kind_t::
PROJECTION;
} else if (text == "scenario") {
value =
demographic_age_sex_source_kind_t::
SCENARIO;
} else if (text == "model_derived") {
value =
demographic_age_sex_source_kind_t::
MODEL_DERIVED;
} else {
return false;
}

return true;
}

[[nodiscard]]
inline bool parse_demographic_geography_basis(
std::string_view text,
demographic_age_sex_geography_basis_t& value
) {
if (text == "direct_province") {
value =
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE;
} else if (
text
== "explicit_geographic_disaggregation"
) {
value =
demographic_age_sex_geography_basis_t::
EXPLICIT_GEOGRAPHIC_DISAGGREGATION;
} else if (
text
== "explicit_parent_geography_proxy"
) {
value =
demographic_age_sex_geography_basis_t::
EXPLICIT_PARENT_GEOGRAPHY_PROXY;
} else {
return false;
}

return true;
}

[[nodiscard]]
inline ProvinceDemographicAgeSexTsvParseResult
parse_province_demographic_age_sex_tsv_row(
std::string_view row
) {
ProvinceDemographicAgeSexTsvParseResult result {};

if (
!row.empty()
&& row.back() == '\r'
) {
row.remove_suffix(1);
}

std::array<
std::string_view,
PROVINCE_DEMOGRAPHIC_TSV_FIELD_COUNT
> fields {};

size_t field_count = 0;
size_t start = 0;

while (true) {
if (
field_count
>= fields.size()
) {
result.status =
province_demographic_tsv_parse_status_t::
WRONG_FIELD_COUNT;

return result;
}

const size_t separator =
row.find('\t', start);

fields[field_count++] =
separator == std::string_view::npos
? row.substr(start)
: row.substr(
start,
separator - start
);

if (
separator
== std::string_view::npos
) {
break;
}

start = separator + 1;
}

if (field_count != fields.size()) {
result.status =
province_demographic_tsv_parse_status_t::
WRONG_FIELD_COUNT;

return result;
}

uint16_t province_index = 0;

if (
!parse_demographic_unsigned_integer(
fields[0],
province_index
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_PROVINCE_INDEX;

return result;
}

result.record.province_index =
province_index_t {
province_index
};

if (
!parse_demographic_source_kind(
fields[1],
result.record.input.source_kind
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_SOURCE_KIND;

return result;
}

if (
!parse_demographic_geography_basis(
fields[2],
result.record.input.geography_basis
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_GEOGRAPHY_BASIS;

return result;
}

if (
!parse_demographic_iso_date(
fields[3],
result.record.input.reference_date
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_REFERENCE_DATE;

return result;
}

if (
!parse_demographic_iso_date(
fields[4],
result.record.input.revision_date
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_REVISION_DATE;

return result;
}

if (
!parse_demographic_unsigned_integer(
fields[5],
result.record.input.authority_priority
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_AUTHORITY_PRIORITY;

return result;
}

if (fields[6].empty()) {
result.status =
province_demographic_tsv_parse_status_t::
EMPTY_SOURCE_IDENTIFIER;

return result;
}

result.record.input.source_identifier =
memory::string {
fields[6].begin(),
fields[6].end()
};

for (
size_t cell_index = 0;
cell_index
< DEMOGRAPHIC_AGE_SEX_CELL_COUNT;
++cell_index
) {
uint32_t weight = 0;

if (
!parse_demographic_unsigned_integer(
fields[
PROVINCE_DEMOGRAPHIC_TSV_METADATA_FIELD_COUNT
+ cell_index
],
weight
)
) {
result.status =
province_demographic_tsv_parse_status_t::
INVALID_WEIGHT;

return result;
}

const size_t age_index =
cell_index / 2;

const bool male =
(cell_index % 2) != 0;

if (male) {
result.record.input.profile
.cohorts[age_index]
.male = weight;
} else {
result.record.input.profile
.cohorts[age_index]
.female = weight;
}
}

if (
!result.record.input.profile
.has_population_shape()
) {
result.status =
province_demographic_tsv_parse_status_t::
EMPTY_PROFILE;

return result;
}

result.status =
province_demographic_tsv_parse_status_t::
PARSED;

return result;
}
}
