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
		class Predictor {
			public:

			si_t m2;
			si_t m1;

			protected:
		};

		const auto PREDICTORS = std::array<Predictor, 2>({
			{ -1, 2 }, // Linear
			{ 0, 1 } // Repeat
		});

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

		auto find_optimal_predictor_for_r(
			cdda::StereoSector& stereo_sector
		) -> size_t {
			auto absolute_differences = std::array<size_t, PREDICTORS.size()>();
			for (auto predictor_index = size_t(0); predictor_index < PREDICTORS.size(); predictor_index += 1) {
				auto& predictor = PREDICTORS.at(predictor_index);
				auto& absolute_difference = absolute_differences.at(predictor_index);
				for (auto sample_index = size_t(2); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
					auto& sample_m2 = stereo_sector.samples[sample_index - 2];
					auto& sample_m1 = stereo_sector.samples[sample_index - 1];
					auto& sample = stereo_sector.samples[sample_index];
					auto prediction = si16_t(predictor.m2 * sample_m2.r.si + predictor.m1 * sample_m1.r.si);
					auto residual = si16_t(sample.r.si - prediction);
					absolute_difference += residual < 0 ? 0 - residual : residual;
				}
			}
			auto absolute_differences_index_of_min_value = size_t(0);
			for (auto absolute_differences_index = size_t(1); absolute_differences_index < absolute_differences.size(); absolute_differences_index += 1) {
				if (absolute_differences.at(absolute_differences_index) < absolute_differences.at(absolute_differences_index_of_min_value)) {
					absolute_differences_index_of_min_value = absolute_differences_index;
				}
			}
			return absolute_differences_index_of_min_value;
		}

		auto find_optimal_predictor_for_l(
			cdda::StereoSector& stereo_sector
		) -> size_t {
			auto absolute_differences = std::array<size_t, PREDICTORS.size()>();
			for (auto predictor_index = size_t(0); predictor_index < PREDICTORS.size(); predictor_index += 1) {
				auto& predictor = PREDICTORS.at(predictor_index);
				auto& absolute_difference = absolute_differences.at(predictor_index);
				for (auto sample_index = size_t(2); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
					auto& sample_m2 = stereo_sector.samples[sample_index - 2];
					auto& sample_m1 = stereo_sector.samples[sample_index - 1];
					auto& sample = stereo_sector.samples[sample_index];
					auto prediction = si16_t(predictor.m2 * sample_m2.l.si + predictor.m1 * sample_m1.l.si);
					auto residual = si16_t(sample.l.si - prediction);
					absolute_difference += residual < 0 ? 0 - residual : residual;
				}
			}
			auto absolute_differences_index_of_min_value = size_t(0);
			for (auto absolute_differences_index = size_t(1); absolute_differences_index < absolute_differences.size(); absolute_differences_index += 1) {
				if (absolute_differences.at(absolute_differences_index) < absolute_differences.at(absolute_differences_index_of_min_value)) {
					absolute_differences_index_of_min_value = absolute_differences_index;
				}
			}
			return absolute_differences_index_of_min_value;
		}

		auto decorrelate_temporally(
			cdda::StereoSector& stereo_sector,
			size_t r_predictor_index,
			size_t l_predictor_index
		) -> void {
			auto r_predictor = PREDICTORS.at(r_predictor_index);
			auto l_predictor = PREDICTORS.at(l_predictor_index);
			for (auto sample_index = cdda::STEREO_SAMPLES_PER_SECTOR - 1; sample_index >= 2; sample_index -= 1) {
				auto& sample_m2 = stereo_sector.samples[sample_index - 2];
				auto& sample_m1 = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				auto r_prediction = si16_t(r_predictor.m2 * sample_m2.r.si + r_predictor.m1 * sample_m1.r.si);
				auto l_prediction = si16_t(l_predictor.m2 * sample_m2.l.si + l_predictor.m1 * sample_m1.l.si);
				sample.r.si -= r_prediction;
				sample.l.si -= l_prediction;
			}
		}

		auto recorrelate_temporally(
			cdda::StereoSector& stereo_sector,
			size_t r_predictor_index,
			size_t l_predictor_index
		) -> void {
			auto r_predictor = PREDICTORS.at(r_predictor_index);
			auto l_predictor = PREDICTORS.at(l_predictor_index);
			for (auto sample_index = size_t(2); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample_m2 = stereo_sector.samples[sample_index - 2];
				auto& sample_m1 = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				auto r_prediction = si16_t(r_predictor.m2 * sample_m2.r.si + r_predictor.m1 * sample_m1.r.si);
				auto l_prediction = si16_t(l_predictor.m2 * sample_m2.l.si + l_predictor.m1 * sample_m1.l.si);
				sample.r.si += r_prediction;
				sample.l.si += l_prediction;
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

		auto compress_sector_using_exponential_golomb_coding(
			const cdda::Sector& sector,
			size_t k,
			size_t r_predictor_index,
			size_t l_predictor_index
		) -> std::vector<byte_t> {
			auto compressed_sector = std::vector<byte_t>();
			compressed_sector.resize(sizeof(LosslessStereoAudioHeader));
			auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(compressed_sector.data());
			header = LosslessStereoAudioHeader();
			header.k = k;
			header.r_predictor_index = r_predictor_index;
			header.l_predictor_index = l_predictor_index;
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
			auto r_predictor_index = find_optimal_predictor_for_r(stereo_sector);
			auto l_predictor_index = find_optimal_predictor_for_l(stereo_sector);
			decorrelate_temporally(stereo_sector, r_predictor_index, l_predictor_index);
			auto& unsigned_sector = *reinterpret_cast<cdda::Sector*>(&sector_data);
			transform_into_optimized_representation(unsigned_sector);
			auto compressed_sectors = std::array<std::vector<byte_t>, 16>();
			for (auto k = size_t(0); k < 16; k += 1) {
				compressed_sectors.at(k) = std::move(compress_sector_using_exponential_golomb_coding(unsigned_sector, k, r_predictor_index, l_predictor_index));
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
			recorrelate_temporally(stereo_sector, header.r_predictor_index, header.l_predictor_index);
			recorrelate_spatially(stereo_sector);
		}

		auto compress_data_using_exponential_golomb_coding(
			byte_t* data,
			size_t size,
			size_t k
		) -> std::vector<byte_t> {
			auto compressed_data = std::vector<byte_t>();
			compressed_data.resize(sizeof(GenericLosslessHeader));
			auto& header = *reinterpret_cast<GenericLosslessHeader*>(compressed_data.data());
			header = GenericLosslessHeader();
			header.k = k;
			auto bitwriter = bits::BitWriter(compressed_data);
			auto power = size_t(1) << k;
			for (auto byte_index = size_t(0); byte_index < size; byte_index += 1) {
				auto byte = data[byte_index];
				auto value = byte + power - 1;
				auto width = sizeof(value) * 8 - std::countl_zero(value + 1);
				bitwriter.append_bits(0, width - 1 - k);
				bitwriter.append_bits(value + 1, width);
			}
			bitwriter.flush_bits();
			return compressed_data;
		}

		auto compress_data_generic_lossless(
			byte_t* data,
			size_t size
		) -> size_t {
			auto compressed_buffers = std::array<std::vector<byte_t>, 8>();
			for (auto k = size_t(0); k < 8; k += 1) {
				compressed_buffers.at(k) = std::move(compress_data_using_exponential_golomb_coding(data, size, k));
			}
			std::sort(compressed_buffers.begin(), compressed_buffers.end(), [](const std::vector<byte_t>& one, const std::vector<byte_t>& two) -> bool_t {
				return one.size() < two.size();
			});
			auto& best = compressed_buffers.front();
			if (best.size() >= cd::SECTOR_LENGTH) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(best.size(), cd::SECTOR_LENGTH));
			}
			std::memcpy(data, best.data(), best.size());
			return best.size();
		}

		auto decompress_data_generic_lossless(
			byte_t* data,
			size_t size,
			size_t compressed_byte_count
		) -> void {
			auto original_data = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(original_data.data(), data, compressed_byte_count);
			auto& header = *reinterpret_cast<GenericLosslessHeader*>(original_data.data());
			auto power = size_t(1) << header.k;
			auto bitreader = bits::BitReader(original_data, header.header_length);
			for (auto byte_index = size_t(0); byte_index < size; byte_index += 1) {
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
				auto byte = value - power;
				data[byte_index] = byte;
			}
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
		if (compression_method == CompressionMethod::GENERIC_LOSSLESS) {
			return internal::compress_data_generic_lossless(sector_data, sizeof(sector_data));
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
		if (compression_method == CompressionMethod::GENERIC_LOSSLESS) {
			return internal::decompress_data_generic_lossless(sector_data, sizeof(sector_data), compressed_byte_count);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto compress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		CompressionMethod::type compression_method
	) -> size_t {
		if (compression_method == CompressionMethod::NONE) {
			return sizeof(subchannels_data);
		}
		if (compression_method == CompressionMethod::LOSSLESS_STEREO_AUDIO) {
			OVERDRIVE_THROW(exceptions::CompressionException("Expected compression method to be supported for data type!"));
		}
		if (compression_method == CompressionMethod::GENERIC_LOSSLESS) {
			return internal::compress_data_generic_lossless(subchannels_data, sizeof(subchannels_data));
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto decompress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		size_t compressed_byte_count,
		CompressionMethod::type compression_method
	) -> void {
		if (compression_method == CompressionMethod::NONE) {
			return;
		}
		if (compression_method == CompressionMethod::LOSSLESS_STEREO_AUDIO) {
			OVERDRIVE_THROW(exceptions::CompressionException("Expected compression method to be supported for data type!"));
		}
		if (compression_method == CompressionMethod::GENERIC_LOSSLESS) {
			return internal::decompress_data_generic_lossless(subchannels_data, sizeof(subchannels_data), compressed_byte_count);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
