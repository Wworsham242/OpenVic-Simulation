#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace OpenVic {

/*
 * Canonical, era-neutral setting capability manifest.
 *
 * The engine knows capability identifiers, not historical eras. A setting,
 * scenario or application package decides which optional mechanisms are
 * active. Universal substrate is not represented here because it is not
 * optional.
 *
 * Capability identifiers are data-facing names. This manifest does not own
 * or implement the mechanisms themselves and therefore creates no second
 * simulation authority.
 */
struct SettingCapabilityManifest final {
	std::string package_id;
	std::vector<std::string> capabilities;

	[[nodiscard]] bool canonicalize() {
		if (package_id.empty()) {
			return false;
		}

		for (std::string const& capability : capabilities) {
			if (capability.empty()) {
				return false;
			}
		}

		std::sort(capabilities.begin(), capabilities.end());

		if (
			std::adjacent_find(
				capabilities.begin(),
				capabilities.end()
			) != capabilities.end()
		) {
			return false;
		}

		return true;
	}

	[[nodiscard]] bool is_canonical() const {
		if (package_id.empty()) {
			return false;
		}

		for (std::string const& capability : capabilities) {
			if (capability.empty()) {
				return false;
			}
		}

		return std::is_sorted(
			capabilities.begin(),
			capabilities.end()
		) && std::adjacent_find(
			capabilities.begin(),
			capabilities.end()
		) == capabilities.end();
	}

	[[nodiscard]] bool has(std::string_view capability) const {
		if (!is_canonical()) {
			return false;
		}

		return std::binary_search(
			capabilities.begin(),
			capabilities.end(),
			capability,
			[](auto const& lhs, auto const& rhs) {
				return std::string_view { lhs } < std::string_view { rhs };
			}
		);
	}

	[[nodiscard]] std::uint64_t checksum() const {
		static constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
		static constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

		auto fold_byte = [](std::uint64_t hash, std::uint8_t byte) {
			hash ^= static_cast<std::uint64_t>(byte);
			hash *= FNV_PRIME;
			return hash;
		};

		auto fold_u64 = [&](std::uint64_t hash, std::uint64_t value) {
			for (unsigned shift = 0; shift < 64; shift += 8) {
				hash = fold_byte(
					hash,
					static_cast<std::uint8_t>((value >> shift) & 0xffu)
				);
			}
			return hash;
		};

		auto fold_string = [&](std::uint64_t hash, std::string_view value) {
			hash = fold_u64(hash, static_cast<std::uint64_t>(value.size()));
			for (unsigned char byte : value) {
				hash = fold_byte(hash, static_cast<std::uint8_t>(byte));
			}
			return hash;
		};

		std::uint64_t hash = FNV_OFFSET;
		hash = fold_string(hash, package_id);
		hash = fold_u64(hash, static_cast<std::uint64_t>(capabilities.size()));

		for (std::string const& capability : capabilities) {
			hash = fold_string(hash, capability);
		}

		return hash;
	}
};

}
