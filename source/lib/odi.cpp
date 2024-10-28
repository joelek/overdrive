#include "odi.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <map>
#include <vector>
#include "bits.h"
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
			cdda::StereoSector& stereo_sector
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample = stereo_sector.samples[sample_index];
				sample.r.si = sample.r.si - sample.l.si;
			}
		}

		auto recorrelate_spatially(
			cdda::StereoSector& stereo_sector
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample = stereo_sector.samples[sample_index];
				sample.r.si = sample.r.si + sample.l.si;
			}
		}

		auto decorrelate_temporally(
			cdda::StereoSector& stereo_sector
		) -> void {
			for (auto sample_index = cdda::STEREO_SAMPLES_PER_SECTOR - 1; sample_index > 0; sample_index -= 1) {
				auto& previous_sample = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				sample.r.si = sample.r.si - previous_sample.r.si;
				sample.l.si = sample.l.si - previous_sample.l.si;
			}
		}

		auto recorrelate_temporally(
			cdda::StereoSector& stereo_sector
		) -> void {
			for (auto sample_index = size_t(1); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& previous_sample = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				sample.r.si = sample.r.si + previous_sample.r.si;
				sample.l.si = sample.l.si + previous_sample.l.si;
			}
		}

		auto transform_into_optimized_representation(
			cdda::Sector& sector
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample = sector.samples[sample_index];
				sample.ui = sample.si < 0 ? 0 - (sample.si << 1) - 1 : sample.si << 1;
			}
		}

		auto transform_from_optimized_representation(
			cdda::Sector& sector
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample = sector.samples[sample_index];
				sample.si = (sample.ui & 1) ? 0 - ((sample.ui + 1) >> 1) : sample.ui >> 1;
			}
		}

		auto compress_using_exponential_golomb_coding(
			const cdda::Sector& sector,
			size_t k
		) -> std::vector<byte_t> {
			auto compressed_sector = std::vector<byte_t>();
			compressed_sector.resize(sizeof(LosslessStereoAudioHeader));
			auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(compressed_sector.data());
			header = LosslessStereoAudioHeader();
			header.k = k;
			auto bitwriter = bits::BitWriter(compressed_sector);
			auto power = size_t(1) << k;
			for (auto sample_index = size_t(0); sample_index < cdda::SAMPLES_PER_SECTOR; sample_index += 1) {
				auto sample = sector.samples[sample_index].ui;
				auto value = sample + power - 1;
				auto width = sizeof(value) * 8 - std::countl_zero(value + 1);
				bitwriter.append_bits(0, width - 1 - k);
				bitwriter.append_bits(value + 1, width);
			}
			bitwriter.flush_bits();
			return compressed_sector;
		}

		auto compress_sector_lossless_stereo_audio(
			array<cd::SECTOR_LENGTH, byte_t>& target_sector_data
		) -> size_t {
			auto sector_data = std::array<byte_t, cd::SECTOR_LENGTH>();
			std::memcpy(&sector_data, &target_sector_data, cd::SECTOR_LENGTH);
			auto& stereo_sector = *reinterpret_cast<cdda::StereoSector*>(&sector_data);
			decorrelate_spatially(stereo_sector);
			decorrelate_temporally(stereo_sector);
			auto& unsigned_sector = *reinterpret_cast<cdda::Sector*>(&sector_data);
			transform_into_optimized_representation(unsigned_sector);
			auto compressed_sectors = std::array<std::vector<byte_t>, 16>();
			for (auto k = size_t(0); k < 16; k += 1) {
				compressed_sectors.at(k) = std::move(compress_using_exponential_golomb_coding(unsigned_sector, k));
			}
			std::sort(compressed_sectors.begin(), compressed_sectors.end(), [](const std::vector<byte_t>& one, const std::vector<byte_t>& two) -> bool_t {
				return one.size() < two.size();
			});
			auto& best = compressed_sectors.front();
			if (best.size() >= cd::SECTOR_LENGTH) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(best.size(), cd::SECTOR_LENGTH));
			}
			std::memcpy(&target_sector_data, best.data(), best.size());
			return best.size();
		}

		auto decompress_sector_lossless_stereo_audio(
			array<cd::SECTOR_LENGTH, byte_t>& target_sector_data,
			size_t compressed_byte_count
		) -> void {
			auto& stereo_sector = *reinterpret_cast<cdda::StereoSector*>(&target_sector_data);
			auto& unsigned_sector = *reinterpret_cast<cdda::Sector*>(&target_sector_data);
			auto compressed_sector = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(compressed_sector.data(), &target_sector_data, compressed_byte_count);
			auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(compressed_sector.data());
			auto power = size_t(1) << header.k;
			auto bitreader = bits::BitReader(compressed_sector, header.header_length);
			for (auto sample_index = size_t(0); sample_index < cdda::SAMPLES_PER_SECTOR; sample_index += 1) {
				auto width = size_t(header.k + 1);
				auto value = size_t(0);
				while (true) {
					value = bitreader.decode_bits(1);
					if (value != 0) {
						break;
					}
					width += 1;
				}
				width -= 1;
				if (width > 0) {
					value = (value << width) | bitreader.decode_bits(width);
				}
				auto sample = value - power;
				unsigned_sector.samples[sample_index].ui = sample;
			}
			transform_from_optimized_representation(unsigned_sector);
			recorrelate_temporally(stereo_sector);
			recorrelate_spatially(stereo_sector);
		}
	}
	}

	auto compress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
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

	auto decompress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
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
