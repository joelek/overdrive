#include "odi.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <vector>
#include "bits.h"
#include "cdda.h"
#include "exceptions.h"

namespace overdrive {
namespace odi {
	auto SectorDataCompressionMethod::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ NONE, "NONE" },
			{ EXPONENTIAL_GOLOMB, "EXPONENTIAL_GOLOMB" },
			{ LOSSLESS_STEREO_AUDIO, "LOSSLESS_STEREO_AUDIO" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	auto SubchannelsDataCompressionMethod::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ NONE, "NONE" },
			{ EXPONENTIAL_GOLOMB, "EXPONENTIAL_GOLOMB" },
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

			si_t m3;
			si_t m2;
			si_t m1;

			protected:
		};

		const auto PREDICTORS = std::array<Predictor, 4>({
			{ 0, 0, 0 }, // None
			{ 0, 0, 1 }, // Repeat
			{ 0, -1, 2 }, // Linear
			{ 1, -3, 3 } // Quadratic
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
				for (auto sample_index = size_t(3); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
					auto& sample_m3 = stereo_sector.samples[sample_index - 3];
					auto& sample_m2 = stereo_sector.samples[sample_index - 2];
					auto& sample_m1 = stereo_sector.samples[sample_index - 1];
					auto& sample = stereo_sector.samples[sample_index];
					auto prediction = si16_t(predictor.m3 * sample_m3.r.si + predictor.m2 * sample_m2.r.si + predictor.m1 * sample_m1.r.si);
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
				for (auto sample_index = size_t(3); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
					auto& sample_m3 = stereo_sector.samples[sample_index - 3];
					auto& sample_m2 = stereo_sector.samples[sample_index - 2];
					auto& sample_m1 = stereo_sector.samples[sample_index - 1];
					auto& sample = stereo_sector.samples[sample_index];
					auto prediction = si16_t(predictor.m3 * sample_m3.l.si + predictor.m2 * sample_m2.l.si + predictor.m1 * sample_m1.l.si);
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
			auto& r_predictor = PREDICTORS.at(r_predictor_index);
			auto& l_predictor = PREDICTORS.at(l_predictor_index);
			for (auto sample_index = cdda::STEREO_SAMPLES_PER_SECTOR - 1; sample_index >= 3; sample_index -= 1) {
				auto& sample_m3 = stereo_sector.samples[sample_index - 3];
				auto& sample_m2 = stereo_sector.samples[sample_index - 2];
				auto& sample_m1 = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				auto r_prediction = si16_t(r_predictor.m3 * sample_m3.r.si + r_predictor.m2 * sample_m2.r.si + r_predictor.m1 * sample_m1.r.si);
				auto l_prediction = si16_t(l_predictor.m3 * sample_m3.l.si + l_predictor.m2 * sample_m2.l.si + l_predictor.m1 * sample_m1.l.si);
				sample.r.si -= r_prediction;
				sample.l.si -= l_prediction;
			}
		}

		auto recorrelate_temporally(
			cdda::StereoSector& stereo_sector,
			size_t r_predictor_index,
			size_t l_predictor_index
		) -> void {
			auto& r_predictor = PREDICTORS.at(r_predictor_index);
			auto& l_predictor = PREDICTORS.at(l_predictor_index);
			for (auto sample_index = size_t(3); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto& sample_m3 = stereo_sector.samples[sample_index - 3];
				auto& sample_m2 = stereo_sector.samples[sample_index - 2];
				auto& sample_m1 = stereo_sector.samples[sample_index - 1];
				auto& sample = stereo_sector.samples[sample_index];
				auto r_prediction = si16_t(r_predictor.m3 * sample_m3.r.si + r_predictor.m2 * sample_m2.r.si + r_predictor.m1 * sample_m1.r.si);
				auto l_prediction = si16_t(l_predictor.m3 * sample_m3.l.si + l_predictor.m2 * sample_m2.l.si + l_predictor.m1 * sample_m1.l.si);
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
			auto& sector = *reinterpret_cast<cdda::Sector*>(&sector_data);
			transform_into_optimized_representation(sector);
			auto samples = reinterpret_cast<ui16_t*>(&sector_data);
			auto compressed_sectors = std::array<std::vector<byte_t>, 16>();
			for (auto k = size_t(0); k < 16; k += 1) {
				auto& compressed_sector = compressed_sectors.at(k);
				compressed_sector.resize(sizeof(LosslessStereoAudioHeader));
				auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(compressed_sector.data());
				header = LosslessStereoAudioHeader();
				header.k = k;
				header.r_predictor_index = r_predictor_index;
				header.l_predictor_index = l_predictor_index;
				auto bitwriter = bits::BitWriter(compressed_sector);
				bits::compress_data_using_exponential_golomb_coding(samples, cdda::SAMPLES_PER_SECTOR, k, bitwriter);
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
			auto& sector = *reinterpret_cast<cdda::Sector*>(&target_sector_data);
			auto samples = reinterpret_cast<ui16_t*>(&target_sector_data);
			auto original = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(original.data(), &target_sector_data, compressed_byte_count);
			auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(original.data());
			auto bitreader = bits::BitReader(original, header.header_length);
			bits::decompress_data_using_exponential_golomb_coding(samples, cdda::SAMPLES_PER_SECTOR, header.k, bitreader);
			transform_from_optimized_representation(sector);
			recorrelate_temporally(stereo_sector, header.r_predictor_index, header.l_predictor_index);
			recorrelate_spatially(stereo_sector);
		}

		auto compress_data_exponential_golomb(
			byte_t* data,
			size_t size
		) -> size_t {
			auto compressed_buffers = std::array<std::vector<byte_t>, 8>();
			for (auto k = size_t(0); k < 8; k += 1) {
				auto& compressed_buffer = compressed_buffers.at(k);
				compressed_buffer.resize(sizeof(ExponentialGolombHeader));
				auto& header = *reinterpret_cast<ExponentialGolombHeader*>(compressed_buffer.data());
				header = ExponentialGolombHeader();
				header.k = k;
				auto bitwriter = bits::BitWriter(compressed_buffer);
				bits::compress_data_using_exponential_golomb_coding(data, size, k, bitwriter);
			}
			std::sort(compressed_buffers.begin(), compressed_buffers.end(), [](const std::vector<byte_t>& one, const std::vector<byte_t>& two) -> bool_t {
				return one.size() < two.size();
			});
			auto& best = compressed_buffers.front();
			if (best.size() >= size) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(best.size(), size));
			}
			std::memcpy(data, best.data(), best.size());
			return best.size();
		}

		auto decompress_data_exponential_golomb(
			byte_t* data,
			size_t size,
			size_t compressed_byte_count
		) -> void {
			auto original = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(original.data(), data, compressed_byte_count);
			auto& header = *reinterpret_cast<ExponentialGolombHeader*>(original.data());
			auto bitreader = bits::BitReader(original, header.header_length);
			bits::decompress_data_using_exponential_golomb_coding(data, size, header.k, bitreader);
		}

		auto do_compress_sector_data(
			array<cd::SECTOR_LENGTH, byte_t>& sector_data,
			SectorDataCompressionMethod::type compression_method
		) -> size_t {
			if (compression_method == SectorDataCompressionMethod::NONE) {
				return sizeof(sector_data);
			}
			if (compression_method == SectorDataCompressionMethod::EXPONENTIAL_GOLOMB) {
				return internal::compress_data_exponential_golomb(sector_data, sizeof(sector_data));
			}
			if (compression_method == SectorDataCompressionMethod::LOSSLESS_STEREO_AUDIO) {
				return internal::compress_sector_lossless_stereo_audio(sector_data);
			}
			OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
		}

		auto do_compress_subchannels_data(
			array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
			SubchannelsDataCompressionMethod::type compression_method
		) -> size_t {
			if (compression_method == SubchannelsDataCompressionMethod::NONE) {
				return sizeof(subchannels_data);
			}
			if (compression_method == SubchannelsDataCompressionMethod::EXPONENTIAL_GOLOMB) {
				return internal::compress_data_exponential_golomb(subchannels_data, sizeof(subchannels_data));
			}
			OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
		}
	}
	}

#ifdef DEBUG
	auto compress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		SectorDataCompressionMethod::type compression_method
	) -> size_t {
		array<cd::SECTOR_LENGTH, byte_t> uncompressed_sector_data;
		std::memcpy(&uncompressed_sector_data, &sector_data, cd::SECTOR_LENGTH);
		auto compressed_byte_count = internal::do_compress_sector_data(sector_data, compression_method);
		array<cd::SECTOR_LENGTH, byte_t> decompressed_sector_data;
		std::memcpy(&decompressed_sector_data, &sector_data, cd::SECTOR_LENGTH);
		decompress_sector_data(decompressed_sector_data, compressed_byte_count, compression_method);
		if (std::memcmp(&decompressed_sector_data, &uncompressed_sector_data, cd::SECTOR_LENGTH) != 0) {
			OVERDRIVE_THROW(exceptions::CompressionValidationError());
		}
		return compressed_byte_count;
	}
#else
	auto compress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		SectorDataCompressionMethod::type compression_method
	) -> size_t {
		return internal::do_compress_sector_data(sector_data, compression_method);
	}
#endif

	auto decompress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		size_t compressed_byte_count,
		SectorDataCompressionMethod::type compression_method
	) -> void {
		if (compression_method == SectorDataCompressionMethod::NONE) {
			return;
		}
		if (compression_method == SectorDataCompressionMethod::EXPONENTIAL_GOLOMB) {
			return internal::decompress_data_exponential_golomb(sector_data, sizeof(sector_data), compressed_byte_count);
		}
		if (compression_method == SectorDataCompressionMethod::LOSSLESS_STEREO_AUDIO) {
			return internal::decompress_sector_lossless_stereo_audio(sector_data, compressed_byte_count);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

#ifdef DEBUG
	auto compress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		SubchannelsDataCompressionMethod::type compression_method
	) -> size_t {
		array<cd::SUBCHANNELS_LENGTH, byte_t> uncompressed_subchannels_data;
		std::memcpy(&uncompressed_subchannels_data, &subchannels_data, cd::SUBCHANNELS_LENGTH);
		auto compressed_byte_count = internal::do_compress_subchannels_data(subchannels_data, compression_method);
		array<cd::SUBCHANNELS_LENGTH, byte_t> decompressed_subchannels_data;
		std::memcpy(&decompressed_subchannels_data, &subchannels_data, cd::SUBCHANNELS_LENGTH);
		decompress_subchannels_data(decompressed_subchannels_data, compressed_byte_count, compression_method);
		if (std::memcmp(&decompressed_subchannels_data, &uncompressed_subchannels_data, cd::SUBCHANNELS_LENGTH) != 0) {
			OVERDRIVE_THROW(exceptions::CompressionValidationError());
		}
		return compressed_byte_count;
	}
#else
	auto compress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		SubchannelsDataCompressionMethod::type compression_method
	) -> size_t {
		return internal::do_compress_subchannels_data(subchannels_data, compression_method);
	}
#endif

	auto decompress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		size_t compressed_byte_count,
		SubchannelsDataCompressionMethod::type compression_method
	) -> void {
		if (compression_method == SubchannelsDataCompressionMethod::NONE) {
			return;
		}
		if (compression_method == SubchannelsDataCompressionMethod::EXPONENTIAL_GOLOMB) {
			return internal::decompress_data_exponential_golomb(subchannels_data, sizeof(subchannels_data), compressed_byte_count);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
