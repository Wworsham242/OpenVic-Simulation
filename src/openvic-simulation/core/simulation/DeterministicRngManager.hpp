#pragma once

#include "openvic-simulation/core/simulation/CampaignStateSnapshot.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace OpenVic {

/// Deterministic named RNG manager.
///
/// Each stable stream id owns an independent xoroshiro128+ state. Streams are seeded
/// deterministically from (master_seed, stream_id), so adding draws to one stream cannot
/// perturb another. This is a simulation primitive, not a cryptographic generator.
class DeterministicRngManager final {
private:
	struct Stream {
		uint64_t state_lo = 0;
		uint64_t state_hi = 0;
		uint64_t draw_count = 0;
	};

	uint64_t master_seed_value = 0;
	std::map<std::string, Stream> streams;

	static constexpr uint64_t rotate_left(uint64_t value, int shift) {
		return (value << shift) | (value >> (64 - shift));
	}

	static uint64_t splitmix64(uint64_t& state) {
		uint64_t z = (state += 0x9e3779b97f4a7c15ull);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
		return z ^ (z >> 31);
	}

	static uint64_t stable_stream_hash(std::string_view stream_id) {
		uint64_t hash = 14695981039346656037ull;
		for (unsigned char byte : stream_id) {
			hash ^= static_cast<uint64_t>(byte);
			hash *= 1099511628211ull;
		}
		return hash;
	}

	[[nodiscard]] Stream make_stream(std::string_view stream_id) const {
		uint64_t seed_state = master_seed_value ^ stable_stream_hash(stream_id);
		Stream stream {
			.state_lo = splitmix64(seed_state),
			.state_hi = splitmix64(seed_state),
			.draw_count = 0
		};

		// xoroshiro128+ must never enter its all-zero absorbing state.
		if (stream.state_lo == 0 && stream.state_hi == 0) {
			stream.state_hi = 0x9e3779b97f4a7c15ull;
		}
		return stream;
	}

	static uint64_t next_from(Stream& stream) {
		const uint64_t s0 = stream.state_lo;
		uint64_t s1 = stream.state_hi;
		const uint64_t result = s0 + s1;

		s1 ^= s0;
		stream.state_lo = rotate_left(s0, 55) ^ s1 ^ (s1 << 14);
		stream.state_hi = rotate_left(s1, 36);
		++stream.draw_count;

		return result;
	}

public:
	explicit DeterministicRngManager(uint64_t master_seed = 0)
		: master_seed_value { master_seed } {}

	[[nodiscard]] uint64_t master_seed() const {
		return master_seed_value;
	}

	[[nodiscard]] uint64_t next_u64(std::string const& stream_id) {
		auto [iterator, inserted] = streams.try_emplace(stream_id);
		if (inserted) {
			iterator->second = make_stream(stream_id);
		}
		return next_from(iterator->second);
	}

	[[nodiscard]] std::optional<uint64_t> draw_count(std::string const& stream_id) const {
		auto const iterator = streams.find(stream_id);
		if (iterator == streams.end()) {
			return std::nullopt;
		}
		return iterator->second.draw_count;
	}

	[[nodiscard]] std::vector<CampaignRngStreamState> capture_state() const {
		std::vector<CampaignRngStreamState> result;
		result.reserve(streams.size());
		for (auto const& [stream_id, stream] : streams) {
			result.push_back(CampaignRngStreamState {
				.stream_id = stream_id,
				.state_lo = stream.state_lo,
				.state_hi = stream.state_hi,
				.draw_count = stream.draw_count
			});
		}
		return result;
	}

	/// Restore is transactional and accepts only canonical strictly sorted stream records.
	[[nodiscard]] bool restore_state(
		uint64_t master_seed,
		std::vector<CampaignRngStreamState> const& saved_streams
	) {
		std::map<std::string, Stream> restored;
		std::string previous;

		for (CampaignRngStreamState const& saved : saved_streams) {
			if (saved.stream_id.empty()) {
				return false;
			}
			if (!previous.empty() && saved.stream_id <= previous) {
				return false;
			}
			if (saved.state_lo == 0 && saved.state_hi == 0) {
				return false;
			}

			restored.emplace(
				saved.stream_id,
				Stream {
					.state_lo = saved.state_lo,
					.state_hi = saved.state_hi,
					.draw_count = saved.draw_count
				}
			);
			previous = saved.stream_id;
		}

		master_seed_value = master_seed;
		streams = std::move(restored);
		return true;
	}
};

}