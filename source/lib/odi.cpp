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
		auto decorrelate_spatially(
			cdda::StereoSample& sample
		) -> void {
			sample.r = sample.r - sample.l;
		}

		auto recorrelate_spatially(
			cdda::StereoSample& sample
		) -> void {
			sample.r = sample.r + sample.l;
		}

		auto decorrelate_temporally(
			const cdda::StereoSample& previous_sample,
			cdda::StereoSample& sample
		) -> void {
			sample.r = sample.r - previous_sample.r;
			sample.l = sample.l - previous_sample.l;
		}

		auto recorrelate_temporally(
			const cdda::StereoSample& previous_sample,
			cdda::StereoSample& sample
		) -> void {
			sample.r = sample.r + previous_sample.r;
			sample.l = sample.l + previous_sample.l;
		}

		auto compress_sector_lossless_stereo_audio(
			array<2352, byte_t>& sector_data
		) -> size_t {
			auto& stereo_sector = *reinterpret_cast<cdda::StereoSector*>(sector_data);
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample = stereo_sector.samples[sample_index];
				decorrelate_spatially(sample);
			}
			for (auto sample_index = cdda::STEREO_SAMPLES_PER_SECTOR - 1; sample_index > 0; sample_index -= 1) {
				auto& previous_sample = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				decorrelate_temporally(previous_sample, sample);
			}
			// TODO: Entropy code.
		}

		auto decompress_sector_lossless_stereo_audio(
			array<2352, byte_t>& sector_data,
			size_t compressed_byte_count
		) -> void {
			// TODO
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

	auto decompress_sector(
		array<2352, byte_t>& sector_data,
		size_t compressed_byte_count,
		CompressionMethod::type compression_method
	) -> void {
		if (compression_method == CompressionMethod::NONE) {
			return;
		}
		if (compression_method == CompressionMethod::LOSSLESS_STEREO_AUDIO) {
			return internal::decompress_sector_lossless_stereo_audio(sector_data, compressed_byte_count);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
