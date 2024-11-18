#include "odi.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <map>
#include <thread>
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
			{ RUN_LENGTH_ENCODING, "RUN_LENGTH_ENCODING" },
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
			{ RUN_LENGTH_ENCODING, "RUN_LENGTH_ENCODING" }
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
			{ 0, 0, 1 }, // Constant extrapolation
			{ 0, -1, 2 }, // Linear extrapolation
			{ 1, -3, 3 } // Quadratic extrapolation
		});

		const auto BITS_PER_PREDICTOR_INDEX = size_t(sizeof(PREDICTORS.size()) * 8 - std::countl_zero(PREDICTORS.size() - 1));
		const auto MAX_RICE_PARAMETER = size_t(16);
		const auto BITS_PER_RICE_PARAMETER = size_t(sizeof(MAX_RICE_PARAMETER) * 8 - std::countl_zero(MAX_RICE_PARAMETER - 1));

		auto deinterleave_channels(
			const cdda::Sector& sector,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample>& channel_a,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample>& channel_b
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				channel_a[sample_index] = sector.samples[sample_index * 2 + 0];
				channel_b[sample_index] = sector.samples[sample_index * 2 + 1];
			}
		}

		auto reinterleave_channels(
			cdda::Sector& sector,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, const cdda::Sample>& channel_a,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, const cdda::Sample>& channel_b
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				sector.samples[sample_index * 2 + 0] = channel_a[sample_index];
				sector.samples[sample_index * 2 + 1] = channel_b[sample_index];
			}
		}

		auto decorrelate_spatially(
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_a,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_b
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				channel_b[sample_index].si -= channel_a[sample_index].si;
			}
		}

		auto recorrelate_spatially(
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_a,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_b
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				channel_b[sample_index].si += channel_a[sample_index].si;
			}
		}

		auto decorrelate_temporally(
			array<cdda::STEREO_SAMPLES_PER_SECTOR, const cdda::Sample>& samples,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample>& residuals,
			const Predictor& predictor
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto sample_index_reverse = cdda::STEREO_SAMPLES_PER_SECTOR - (sample_index + 1);
				auto prediction = 0;
				if (sample_index_reverse > 0) {
					auto sample_m3 = samples[sample_index_reverse >= 3 ? sample_index_reverse - 3 : 0].si;
					auto sample_m2 = samples[sample_index_reverse >= 2 ? sample_index_reverse - 2 : 0].si;
					auto sample_m1 = samples[sample_index_reverse >= 1 ? sample_index_reverse - 1 : 0].si;
					prediction = predictor.m3 * sample_m3 + predictor.m2 * sample_m2 + predictor.m1 * sample_m1;
				}
				auto sample = samples[sample_index_reverse].si;
				auto residual = si16_t(sample - prediction);
				residuals[sample_index_reverse].si = residual;
			}
		}

		auto recorrelate_temporally(
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample>& samples,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, const cdda::Sample>& residuals,
			const Predictor& predictor
		) -> void {
			for (auto sample_index = size_t(0); sample_index < cdda::STEREO_SAMPLES_PER_SECTOR; sample_index += 1) {
				auto prediction = 0;
				if (sample_index > 0) {
					auto sample_m3 = samples[sample_index >= 3 ? sample_index - 3 : 0].si;
					auto sample_m2 = samples[sample_index >= 2 ? sample_index - 2 : 0].si;
					auto sample_m1 = samples[sample_index >= 1 ? sample_index - 1 : 0].si;
					prediction = predictor.m3 * sample_m3 + predictor.m2 * sample_m2 + predictor.m1 * sample_m1;
				}
				auto residual = residuals[sample_index].si;
				auto sample = si16_t(residual + prediction);
				samples[sample_index].si = sample;
			}
		}

		auto compress_sector_lossless_stereo_audio_channel_with_parameters(
			bits::BitWriter bitwriter,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, const cdda::Sample>& samples,
			size_t rice_parameter,
			size_t predictor_index
		) -> bits::BitWriter {
			auto& predictor = PREDICTORS.at(predictor_index);
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> residuals;
			decorrelate_temporally(samples, residuals, predictor);
			try {
				bitwriter.append_bits(rice_parameter, BITS_PER_RICE_PARAMETER);
				bitwriter.append_bits(predictor_index, BITS_PER_PREDICTOR_INDEX);
				bits::compress_data_using_rice_coding(reinterpret_cast<si16_t*>(&residuals), cdda::STEREO_SAMPLES_PER_SECTOR, rice_parameter, bitwriter);
			} catch (const exceptions::BitWriterSizeExceededError& e) {}
			return bitwriter;
		}

		auto compress_sector_lossless_stereo_audio_channel(
			bits::BitWriter bitwriter,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, const cdda::Sample>& samples
		) -> bits::BitWriter {
			auto thread_bitwriters = std::array<bits::BitWriter, MAX_RICE_PARAMETER>();
			auto threads = std::array<std::thread, MAX_RICE_PARAMETER>();
			for (auto thread_index = size_t(0); thread_index < MAX_RICE_PARAMETER; thread_index += 1) {
				auto thread = std::thread([&, thread_index]() -> void {
					auto rice_parameter = thread_index;
					auto bitwriters = std::array<bits::BitWriter, PREDICTORS.size()>();
					for (auto predictor_index = size_t(0); predictor_index < bitwriters.size(); predictor_index += 1) {
						bitwriters.at(predictor_index) = std::move(compress_sector_lossless_stereo_audio_channel_with_parameters(bitwriter, samples, rice_parameter, predictor_index));
					}
					auto best_bitwriter_index = size_t(0);
					for (auto bitwriter_index = size_t(1); bitwriter_index < bitwriters.size(); bitwriter_index += 1) {
						if (bitwriters.at(bitwriter_index).get_size() < bitwriters.at(best_bitwriter_index).get_size()) {
							best_bitwriter_index = bitwriter_index;
						}
					}
					auto& best_bitwriter = bitwriters.at(best_bitwriter_index);
					thread_bitwriters.at(thread_index) = std::move(best_bitwriter);
				});
				threads.at(thread_index) = std::move(thread);
			}
			for (auto thread_index = size_t(0); thread_index < threads.size(); thread_index += 1) {
				threads.at(thread_index).join();
			}
			auto best_bitwriter_index = size_t(0);
			for (auto thread_bitwriter_index = size_t(1); thread_bitwriter_index < thread_bitwriters.size(); thread_bitwriter_index += 1) {
				if (thread_bitwriters.at(thread_bitwriter_index).get_size() < thread_bitwriters.at(best_bitwriter_index).get_size()) {
					best_bitwriter_index = thread_bitwriter_index;
				}
			}
			auto& best_bitwriter = thread_bitwriters.at(best_bitwriter_index);
			return best_bitwriter;
		}

		auto compress_sector_lossless_stereo_audio(
			array<cd::SECTOR_LENGTH, byte_t>& target_sector_data
		) -> size_t {
			auto& sector = *reinterpret_cast<cdda::Sector*>(&target_sector_data);
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_a;
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_b;
			deinterleave_channels(sector, channel_a, channel_b);
			decorrelate_spatially(channel_a, channel_b);
			auto bitwriter = bits::BitWriter(cd::SECTOR_LENGTH);
			bitwriter = std::move(compress_sector_lossless_stereo_audio_channel(bitwriter, channel_a));
			bitwriter = std::move(compress_sector_lossless_stereo_audio_channel(bitwriter, channel_b));
			try {
				bitwriter.flush_bits();
			} catch (const exceptions::BitWriterSizeExceededError& e) {}
			auto& buffer = bitwriter.get_buffer();
			if (buffer.size() >= cd::SECTOR_LENGTH) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(buffer.size(), cd::SECTOR_LENGTH));
			}
			std::memcpy(&target_sector_data, buffer.data(), buffer.size());
			return buffer.size();
		}

		auto decompress_sector_lossless_stereo_audio_channel(
			bits::BitReader& bitreader,
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample>& samples
		) -> void {
			auto rice_parameter = bitreader.decode_bits(BITS_PER_RICE_PARAMETER);
			auto predictor_index = bitreader.decode_bits(BITS_PER_PREDICTOR_INDEX);
			auto& predictor = PREDICTORS.at(predictor_index);
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> residuals;
			bits::decompress_data_using_rice_coding(reinterpret_cast<si16_t*>(&residuals), cdda::STEREO_SAMPLES_PER_SECTOR, rice_parameter, bitreader);
			recorrelate_temporally(samples, residuals, predictor);
		}

		auto decompress_sector_lossless_stereo_audio(
			array<cd::SECTOR_LENGTH, byte_t>& target_sector_data,
			size_t compressed_byte_count
		) -> void {
			auto& sector = *reinterpret_cast<cdda::Sector*>(&target_sector_data);
			auto original = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(original.data(), &target_sector_data, compressed_byte_count);
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_a;
			array<cdda::STEREO_SAMPLES_PER_SECTOR, cdda::Sample> channel_b;
			auto bitreader = bits::BitReader(original, 0);
			decompress_sector_lossless_stereo_audio_channel(bitreader, channel_a);
			decompress_sector_lossless_stereo_audio_channel(bitreader, channel_b);
			recorrelate_spatially(channel_a, channel_b);
			reinterleave_channels(sector, channel_a, channel_b);
		}

		auto compress_run_length_encoding(
			byte_t* target_data,
			size_t target_size
		) -> size_t {
			auto bitwriter = bits::BitWriter(target_size);
			try {
				bits::compress_data_using_rle_coding(target_data, target_size, bitwriter);
				bitwriter.flush_bits();
			} catch (exceptions::BitWriterSizeExceededError& e) {}
			auto& buffer = bitwriter.get_buffer();
			if (buffer.size() >= target_size) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(buffer.size(), target_size));
			}
			std::memcpy(target_data, buffer.data(), buffer.size());
			return buffer.size();
		}

		auto decompress_run_length_encoding(
			byte_t* target_data,
			size_t target_size,
			size_t compressed_byte_count
		) -> void {
			auto original = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(original.data(), target_data, compressed_byte_count);
			auto bitreader = bits::BitReader(original, 0);
			bits::decompress_data_using_rle_coding(target_data, target_size, bitreader);
		}

		auto do_compress_sector_data(
			array<cd::SECTOR_LENGTH, byte_t>& sector_data,
			SectorDataCompressionMethod::type compression_method
		) -> size_t {
			if (compression_method == SectorDataCompressionMethod::NONE) {
				return sizeof(sector_data);
			}
			if (compression_method == SectorDataCompressionMethod::RUN_LENGTH_ENCODING) {
				return compress_run_length_encoding(sector_data, cd::SECTOR_LENGTH);
			}
			if (compression_method == SectorDataCompressionMethod::LOSSLESS_STEREO_AUDIO) {
				return compress_sector_lossless_stereo_audio(sector_data);
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
			if (compression_method == SectorDataCompressionMethod::RUN_LENGTH_ENCODING) {
				return compress_run_length_encoding(subchannels_data, cd::SUBCHANNELS_LENGTH);
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
		if (compression_method == SectorDataCompressionMethod::RUN_LENGTH_ENCODING) {
			return internal::decompress_run_length_encoding(sector_data, cd::SECTOR_LENGTH, compressed_byte_count);
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
		if (compression_method == SectorDataCompressionMethod::RUN_LENGTH_ENCODING) {
			return internal::decompress_run_length_encoding(subchannels_data, cd::SUBCHANNELS_LENGTH, compressed_byte_count);
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
