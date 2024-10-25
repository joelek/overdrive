#include "odi.h"

#include <map>
#include "cdda.h"
#include "exceptions.h"

namespace overdrive {
namespace odi {
	auto CompressionMethod::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ NONE, "NONE" },
			{ LOSSLESS_STEREO_AUDIO, "LOSSLESS_STEREO_AUDIO" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	auto Readability::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ UNREADABLE, "UNREADABLE" },
			{ READABLE, "READABLE" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	namespace internal {
	namespace {
		auto encode_xy_channels(
			si16_t l,
			si16_t r,
			si16_t& x,
			si16_t& y
		) -> void {
			x = l;
			y = r - l;
		}

		auto decode_xy_channels(
			si16_t& l,
			si16_t& r,
			si16_t x,
			si16_t y
		) -> void {
			l = x;
			r = x + y;
		}

		auto compress_sector_lossless_stereo_audio(
			array<2352, byte_t>& sector_data
		) -> size_t {
			auto& stereo_sector = *reinterpret_cast<cdda::StereoSector*>(sector_data);
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample = stereo_sector.samples[sample_index];
				auto l = sample[0];
				auto r = sample[1];
				auto x = si16_t();
				auto y = si16_t();
				encode_xy_channels(l, r, x, y);
				// TODO: Compute channel-wise temporal delta.
				// TODO: Entropy code.
			}
		}
	}
	}

	auto compress_sector(
		array<2352, byte_t>& sector_data,
		CompressionMethod::type compression_method
	) -> size_t {
		if (compression_method == CompressionMethod::NONE) {
			return sizeof(sector_data);
		}
		if (compression_method == CompressionMethod::LOSSLESS_STEREO_AUDIO) {
			return internal::compress_sector_lossless_stereo_audio(sector_data);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
