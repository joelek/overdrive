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
			{ 0, 0, 1 }, // Constant extrapolation
			{ 0, -1, 2 }, // Linear extrapolation
			{ 1, -3, 3 } // Quadratic extrapolation
		});

		const auto SAMPLES_PER_GROUP = size_t(1);
		const auto GROUPS_PER_SECTOR = size_t((cdda::STEREO_SAMPLES_PER_SECTOR + SAMPLES_PER_GROUP - 1) / SAMPLES_PER_GROUP);
		const auto BITS_PER_PREDICTOR_INDEX = size_t(sizeof(PREDICTORS.size()) * 8 - std::countl_zero(PREDICTORS.size() - 1));

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

		auto find_optimal_group_predictors(
			cdda::StereoSector& stereo_sector,
			array<GROUPS_PER_SECTOR, byte_t>& group_predictor_indices_l,
			array<GROUPS_PER_SECTOR, byte_t>& group_predictor_indices_r
		) -> void {
			for (auto group_index = size_t(0); group_index < GROUPS_PER_SECTOR; group_index += 1) {
				auto first_sample = group_index * SAMPLES_PER_GROUP;
				auto last_sample = first_sample + SAMPLES_PER_GROUP;
				last_sample = last_sample < cdda::STEREO_SAMPLES_PER_SECTOR ? last_sample : cdda::STEREO_SAMPLES_PER_SECTOR;
				auto absolute_differences_l = std::array<size_t, PREDICTORS.size()>();
				auto absolute_differences_r = std::array<size_t, PREDICTORS.size()>();
				for (auto predictor_index = size_t(0); predictor_index < PREDICTORS.size(); predictor_index += 1) {
					auto& predictor = PREDICTORS.at(predictor_index);
					auto absolute_difference_l = size_t(0);
					auto absolute_difference_r = size_t(0);
					for (auto sample_index = first_sample; sample_index < last_sample; sample_index += 1) {
						if (sample_index < 3) {
							continue;
						}
						auto& sample_m3 = stereo_sector.samples[sample_index - 3];
						auto& sample_m2 = stereo_sector.samples[sample_index - 2];
						auto& sample_m1 = stereo_sector.samples[sample_index - 1];
						auto& sample = stereo_sector.samples[sample_index];
						auto prediction_l = (predictor.m3 * sample_m3.l.si + predictor.m2 * sample_m2.l.si + predictor.m1 * sample_m1.l.si);
						auto residual_l = si16_t(sample.l.si - prediction_l);
						absolute_difference_l += residual_l < 0 ? 0 - residual_l : residual_l;
						auto prediction_r = (predictor.m3 * sample_m3.r.si + predictor.m2 * sample_m2.r.si + predictor.m1 * sample_m1.r.si);
						auto residual_r = si16_t(sample.r.si - prediction_r);
						absolute_difference_r += residual_r < 0 ? 0 - residual_r : residual_r;
					}
					absolute_differences_l.at(predictor_index) = absolute_difference_l;
					absolute_differences_r.at(predictor_index) = absolute_difference_r;
				}
				auto group_predictor_index_l = size_t(0);
				for (auto absolute_differences_l_index = size_t(1); absolute_differences_l_index < absolute_differences_l.size(); absolute_differences_l_index += 1) {
					if (absolute_differences_l.at(absolute_differences_l_index) < absolute_differences_l.at(group_predictor_index_l)) {
						group_predictor_index_l = absolute_differences_l_index;
					}
				}
				group_predictor_indices_l[group_index] = group_predictor_index_l;
				auto group_predictor_index_r = size_t(0);
				for (auto absolute_differences_r_index = size_t(1); absolute_differences_r_index < absolute_differences_r.size(); absolute_differences_r_index += 1) {
					if (absolute_differences_r.at(absolute_differences_r_index) < absolute_differences_r.at(group_predictor_index_r)) {
						group_predictor_index_r = absolute_differences_r_index;
					}
				}
				group_predictor_indices_r[group_index] = group_predictor_index_r;
			}
		}

		auto decorrelate_temporally(
			cdda::StereoSector& stereo_sector,
			array<GROUPS_PER_SECTOR, byte_t>& group_predictor_indices_l,
			array<GROUPS_PER_SECTOR, byte_t>& group_predictor_indices_r
		) -> void {
			for (auto group_index = si_t(GROUPS_PER_SECTOR - 1); group_index >= si_t(0); group_index -= 1) {
				auto first_sample = group_index * SAMPLES_PER_GROUP;
				auto last_sample = first_sample + SAMPLES_PER_GROUP < cdda::STEREO_SAMPLES_PER_SECTOR ? first_sample + SAMPLES_PER_GROUP : cdda::STEREO_SAMPLES_PER_SECTOR;
				auto& l_predictor = PREDICTORS.at(group_predictor_indices_l[group_index]);
				auto& r_predictor = PREDICTORS.at(group_predictor_indices_r[group_index]);
				for (auto sample_index = si_t(last_sample - 1); sample_index >= si_t(first_sample); sample_index -= 1) {
					auto l_prediction = 0;
					auto r_prediction = 0;
					if (sample_index > 0) {
						auto& sample_m3 = stereo_sector.samples[sample_index >= 3 ? sample_index - 3 : 0];
						auto& sample_m2 = stereo_sector.samples[sample_index >= 2 ? sample_index - 2 : 0];
						auto& sample_m1 = stereo_sector.samples[sample_index >= 1 ? sample_index - 1 : 0];
						l_prediction = l_predictor.m3 * sample_m3.l.si + l_predictor.m2 * sample_m2.l.si + l_predictor.m1 * sample_m1.l.si;
						r_prediction = r_predictor.m3 * sample_m3.r.si + r_predictor.m2 * sample_m2.r.si + r_predictor.m1 * sample_m1.r.si;
					}
					auto& sample = stereo_sector.samples[sample_index];
					sample.l.si -= l_prediction;
					sample.r.si -= r_prediction;
				}
			}
		}

		auto recorrelate_temporally(
			cdda::StereoSector& stereo_sector,
			array<GROUPS_PER_SECTOR, byte_t>& group_predictor_indices_l,
			array<GROUPS_PER_SECTOR, byte_t>& group_predictor_indices_r
		) -> void {
			for (auto group_index = size_t(0); group_index < GROUPS_PER_SECTOR; group_index += 1) {
				auto first_sample = group_index * SAMPLES_PER_GROUP;
				auto last_sample = first_sample + SAMPLES_PER_GROUP < cdda::STEREO_SAMPLES_PER_SECTOR ? first_sample + SAMPLES_PER_GROUP : cdda::STEREO_SAMPLES_PER_SECTOR;
				auto& l_predictor = PREDICTORS.at(group_predictor_indices_l[group_index]);
				auto& r_predictor = PREDICTORS.at(group_predictor_indices_r[group_index]);
				for (auto sample_index = first_sample; sample_index < last_sample; sample_index += 1) {
					auto l_prediction = 0;
					auto r_prediction = 0;
					if (sample_index > 0) {
						auto& sample_m3 = stereo_sector.samples[sample_index >= 3 ? sample_index - 3 : 0];
						auto& sample_m2 = stereo_sector.samples[sample_index >= 2 ? sample_index - 2 : 0];
						auto& sample_m1 = stereo_sector.samples[sample_index >= 1 ? sample_index - 1 : 0];
						l_prediction = l_predictor.m3 * sample_m3.l.si + l_predictor.m2 * sample_m2.l.si + l_predictor.m1 * sample_m1.l.si;
						r_prediction = r_predictor.m3 * sample_m3.r.si + r_predictor.m2 * sample_m2.r.si + r_predictor.m1 * sample_m1.r.si;
					}
					auto& sample = stereo_sector.samples[sample_index];
					sample.l.si += l_prediction;
					sample.r.si += r_prediction;
				}
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
			array<GROUPS_PER_SECTOR, byte_t> group_predictor_indices_l;
			array<GROUPS_PER_SECTOR, byte_t> group_predictor_indices_r;
			find_optimal_group_predictors(stereo_sector, group_predictor_indices_l, group_predictor_indices_r);
			decorrelate_temporally(stereo_sector, group_predictor_indices_l, group_predictor_indices_r);
			auto& sector = *reinterpret_cast<cdda::Sector*>(&sector_data);
			transform_into_optimized_representation(sector);
			auto samples = reinterpret_cast<ui16_t*>(&sector_data);
			auto bitwriters = std::array<bits::BitWriter, 16>();
			auto threads = std::array<std::thread, 16>();
			for (auto k = size_t(0); k < 16; k += 1) {
				auto thread = std::thread([&, k]() -> void {
					auto& bitwriter = bitwriters.at(k);
					bitwriter = std::move(bits::BitWriter(cd::SECTOR_LENGTH));
					auto& buffer = bitwriter.get_buffer();
					buffer.resize(sizeof(LosslessStereoAudioHeader));
					auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(buffer.data());
					header = LosslessStereoAudioHeader();
					header.k = k;
					try {
						for (auto group_index = size_t(0); group_index < GROUPS_PER_SECTOR; group_index += 1) {
							auto group_predictor_index_l = group_predictor_indices_l[group_index];
							auto group_predictor_index_r = group_predictor_indices_r[group_index];
							bitwriter.append_bits(group_predictor_index_l, BITS_PER_PREDICTOR_INDEX);
							bitwriter.append_bits(group_predictor_index_r, BITS_PER_PREDICTOR_INDEX);
						}
						bits::compress_data_using_rice_coding(samples, cdda::SAMPLES_PER_SECTOR, k, bitwriter);
					} catch (const exceptions::BitWriterSizeExceededError& e) {}
					try {
						bitwriter.flush_bits();
					} catch (const exceptions::BitWriterSizeExceededError& e) {}
				});
				threads.at(k) = std::move(thread);
			}
			for (auto thread_index = size_t(0); thread_index < threads.size(); thread_index += 1) {
				threads.at(thread_index).join();
			}
			std::sort(bitwriters.begin(), bitwriters.end(), [](const bits::BitWriter& one, const bits::BitWriter& two) -> bool_t {
				return one.get_size() < two.get_size();
			});
			auto& best = bitwriters.front();
			if (best.get_buffer().size() >= cd::SECTOR_LENGTH) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(best.get_buffer().size(), cd::SECTOR_LENGTH));
			}
			std::memcpy(&target_sector_data, best.get_buffer().data(), best.get_buffer().size());
			return best.get_buffer().size();
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
			array<GROUPS_PER_SECTOR, byte_t> group_predictor_indices_l;
			array<GROUPS_PER_SECTOR, byte_t> group_predictor_indices_r;
			for (auto group_index = size_t(0); group_index < GROUPS_PER_SECTOR; group_index += 1) {
				group_predictor_indices_l[group_index] = bitreader.decode_bits(BITS_PER_PREDICTOR_INDEX);
				group_predictor_indices_r[group_index] = bitreader.decode_bits(BITS_PER_PREDICTOR_INDEX);
			}
			bits::decompress_data_using_rice_coding(samples, cdda::SAMPLES_PER_SECTOR, header.k, bitreader);
			transform_from_optimized_representation(sector);
			recorrelate_temporally(stereo_sector, group_predictor_indices_l, group_predictor_indices_r);
			recorrelate_spatially(stereo_sector);
		}

		auto compress_data_exponential_golomb(
			byte_t* data,
			size_t size
		) -> size_t {
			auto bitwriters = std::array<bits::BitWriter, 8>();
			for (auto k = size_t(0); k < 8; k += 1) {
				auto& bitwriter = bitwriters.at(k);
				bitwriter = std::move(bits::BitWriter(size));
				auto& buffer = bitwriter.get_buffer();
				buffer.resize(sizeof(ExponentialGolombHeader));
				auto& header = *reinterpret_cast<ExponentialGolombHeader*>(buffer.data());
				header = ExponentialGolombHeader();
				header.k = k;
				try {
					bits::compress_data_using_exponential_golomb_coding(data, size, k, bitwriter);
				} catch (const exceptions::BitWriterSizeExceededError& e) {}
				try {
					bitwriter.flush_bits();
				} catch (const exceptions::BitWriterSizeExceededError& e) {}
			}
			std::sort(bitwriters.begin(), bitwriters.end(), [](const bits::BitWriter& one, const bits::BitWriter& two) -> bool_t {
				return one.get_size() < two.get_size();
			});
			auto& best = bitwriters.front();
			if (best.get_buffer().size() >= size) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(best.get_buffer().size(), size));
			}
			std::memcpy(data, best.get_buffer().data(), best.get_buffer().size());
			return best.get_buffer().size();
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

		auto decompress_sector_lossless_stereo_audio_opt(
			array<cd::SECTOR_LENGTH, byte_t>& target_sector_data,
			size_t compressed_byte_count
		) -> void {
			auto& stereo_sector = *reinterpret_cast<cdda::StereoSector*>(&target_sector_data);
			auto original = std::vector<byte_t>(compressed_byte_count);
			std::memcpy(original.data(), &target_sector_data, compressed_byte_count);
			auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(original.data());
			auto bitreader = bits::BitReader(original, header.header_length);
			for (auto group_index = size_t(0); group_index < GROUPS_PER_SECTOR; group_index += 1) {
				auto first_sample = group_index * SAMPLES_PER_GROUP;
				auto last_sample = first_sample + SAMPLES_PER_GROUP < cdda::STEREO_SAMPLES_PER_SECTOR ? first_sample + SAMPLES_PER_GROUP : cdda::STEREO_SAMPLES_PER_SECTOR;
				{
					auto predictor_index = bitreader.decode_bits(BITS_PER_PREDICTOR_INDEX);
					auto& predictor = PREDICTORS.at(predictor_index);
					for (auto sample_index = first_sample; sample_index < last_sample; sample_index += 1) {
						auto prediction = 0;
						if (sample_index > 0) {
							auto sample_m3 = stereo_sector.samples[sample_index >= 3 ? sample_index - 3 : 0].l.si;
							auto sample_m2 = stereo_sector.samples[sample_index >= 2 ? sample_index - 2 : 0].l.si;
							auto sample_m1 = stereo_sector.samples[sample_index >= 1 ? sample_index - 1 : 0].l.si;
							prediction = predictor.m3 * sample_m3 + predictor.m2 * sample_m2 + predictor.m1 * sample_m1;
						}
						auto decoded = ui16_t(0);
						bits::decompress_data_using_rice_coding(&decoded, 1, header.k, bitreader);
						auto residual = si16_t(decoded & 1) ? 0 - ((decoded + 1) >> 1) : decoded >> 1;
						auto sample = si16_t(residual + prediction);
						stereo_sector.samples[sample_index].l.si = sample;
					}
				}
				{
					auto predictor_index = bitreader.decode_bits(BITS_PER_PREDICTOR_INDEX);
					auto& predictor = PREDICTORS.at(predictor_index);
					for (auto sample_index = first_sample; sample_index < last_sample; sample_index += 1) {
						auto prediction = 0;
						if (sample_index > 0) {
							auto sample_m3 = stereo_sector.samples[sample_index >= 3 ? sample_index - 3 : 0].r.si;
							auto sample_m2 = stereo_sector.samples[sample_index >= 2 ? sample_index - 2 : 0].r.si;
							auto sample_m1 = stereo_sector.samples[sample_index >= 1 ? sample_index - 1 : 0].r.si;
							prediction = predictor.m3 * sample_m3 + predictor.m2 * sample_m2 + predictor.m1 * sample_m1;
						}
						auto decoded = ui16_t(0);
						bits::decompress_data_using_rice_coding(&decoded, 1, header.k, bitreader);
						auto residual = si16_t(decoded & 1) ? 0 - ((decoded + 1) >> 1) : decoded >> 1;
						auto sample = si16_t(residual + prediction);
						stereo_sector.samples[sample_index].r.si = sample;
					}
				}
			}
			recorrelate_spatially(stereo_sector);
		}

		auto compress_sector_lossless_stereo_audio_opt(
			array<cd::SECTOR_LENGTH, byte_t>& target_sector_data
		) -> size_t {
			auto sector_data = std::array<byte_t, cd::SECTOR_LENGTH>();
			std::memcpy(&sector_data, &target_sector_data, cd::SECTOR_LENGTH);
			auto& stereo_sector = *reinterpret_cast<cdda::StereoSector*>(&sector_data);
			decorrelate_spatially(stereo_sector);
			auto bitwriters = std::array<bits::BitWriter, 16>();
			auto threads = std::array<std::thread, 16>();
			for (auto k = size_t(0); k < 16; k += 1) {
				auto thread = std::thread([&, k]() -> void {
					auto& bitwriter = bitwriters.at(k);
					bitwriter = bits::BitWriter(cd::SECTOR_LENGTH);
					auto& buffer = bitwriter.get_buffer();
					buffer.resize(sizeof(LosslessStereoAudioHeader));
					auto& header = *reinterpret_cast<LosslessStereoAudioHeader*>(buffer.data());
					header = LosslessStereoAudioHeader();
					header.k = k;
					auto& shortest_coding = bitwriter;
					for (auto group_index = size_t(0); group_index < GROUPS_PER_SECTOR; group_index += 1) {
						auto first_sample = group_index * SAMPLES_PER_GROUP;
						auto last_sample = first_sample + SAMPLES_PER_GROUP < cdda::STEREO_SAMPLES_PER_SECTOR ? first_sample + SAMPLES_PER_GROUP : cdda::STEREO_SAMPLES_PER_SECTOR;
						{
							auto bitwriters = std::array<bits::BitWriter, PREDICTORS.size()>();
							for (auto predictor_index = size_t(0); predictor_index < PREDICTORS.size(); predictor_index += 1) {
								auto& predictor = PREDICTORS.at(predictor_index);
								auto& bitwriter = bitwriters.at(predictor_index);
								bitwriter = std::move(bits::BitWriter(shortest_coding));
								try {
									bitwriter.append_bits(predictor_index, BITS_PER_PREDICTOR_INDEX);
									for (auto sample_index = first_sample; sample_index < last_sample; sample_index += 1) {
										auto sample = stereo_sector.samples[sample_index].l.si;
										auto prediction = 0;
										if (sample_index > 0) {
											auto sample_m3 = stereo_sector.samples[sample_index >= 3 ? sample_index - 3 : 0].l.si;
											auto sample_m2 = stereo_sector.samples[sample_index >= 2 ? sample_index - 2 : 0].l.si;
											auto sample_m1 = stereo_sector.samples[sample_index >= 1 ? sample_index - 1 : 0].l.si;
											prediction = predictor.m3 * sample_m3 + predictor.m2 * sample_m2 + predictor.m1 * sample_m1;
										}
										auto residual = si16_t(sample - prediction);
										auto encoded = ui16_t(residual < 0 ? 0 - (residual << 1) - 1 : residual << 1);
										bits::compress_data_using_rice_coding(&encoded, 1, k, bitwriter);
									}
								} catch (const exceptions::BitWriterSizeExceededError& e) {}
							}
							std::sort(bitwriters.begin(), bitwriters.end(), [](const bits::BitWriter& one, const bits::BitWriter& two) -> bool_t {
								return one.get_size() < two.get_size();
							});
							auto& best = bitwriters.front();
							shortest_coding = std::move(best);
						}
						{
							auto bitwriters = std::array<bits::BitWriter, PREDICTORS.size()>();
							for (auto predictor_index = size_t(0); predictor_index < PREDICTORS.size(); predictor_index += 1) {
								auto& predictor = PREDICTORS.at(predictor_index);
								auto& bitwriter = bitwriters.at(predictor_index);
								bitwriter = std::move(bits::BitWriter(shortest_coding));
								try {
									bitwriter.append_bits(predictor_index, BITS_PER_PREDICTOR_INDEX);
									for (auto sample_index = first_sample; sample_index < last_sample; sample_index += 1) {
										auto sample = stereo_sector.samples[sample_index].r.si;
										auto prediction = 0;
										if (sample_index > 0) {
											auto sample_m3 = stereo_sector.samples[sample_index >= 3 ? sample_index - 3 : 0].r.si;
											auto sample_m2 = stereo_sector.samples[sample_index >= 2 ? sample_index - 2 : 0].r.si;
											auto sample_m1 = stereo_sector.samples[sample_index >= 1 ? sample_index - 1 : 0].r.si;
											prediction = predictor.m3 * sample_m3 + predictor.m2 * sample_m2 + predictor.m1 * sample_m1;
										}
										auto residual = si16_t(sample - prediction);
										auto encoded = ui16_t(residual < 0 ? 0 - (residual << 1) - 1 : residual << 1);
										bits::compress_data_using_rice_coding(&encoded, 1, k, bitwriter);
									}
								} catch (const exceptions::BitWriterSizeExceededError& e) {}
							}
							std::sort(bitwriters.begin(), bitwriters.end(), [](const bits::BitWriter& one, const bits::BitWriter& two) -> bool_t {
								return one.get_size() < two.get_size();
							});
							auto& best = bitwriters.front();
							shortest_coding = std::move(best);
						}
					}
					try {
						shortest_coding.flush_bits();
					} catch (const exceptions::BitWriterSizeExceededError& e) {}
				});
				threads.at(k) = std::move(thread);
			}
			for (auto thread_index = size_t(0); thread_index < threads.size(); thread_index += 1) {
				threads.at(thread_index).join();
			}
			std::sort(bitwriters.begin(), bitwriters.end(), [](const bits::BitWriter& one, const bits::BitWriter& two) -> bool_t {
				return one.get_size() < two.get_size();
			});
			auto& best = bitwriters.front();
			if (best.get_buffer().size() >= cd::SECTOR_LENGTH) {
				OVERDRIVE_THROW(exceptions::CompressedSizeExceededUncompressedSizeException(best.get_buffer().size(), cd::SECTOR_LENGTH));
			}
			std::memcpy(&target_sector_data, best.get_buffer().data(), best.get_buffer().size());
			return best.get_buffer().size();
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
				return internal::compress_sector_lossless_stereo_audio_opt(sector_data);
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
			return internal::decompress_sector_lossless_stereo_audio_opt(sector_data, compressed_byte_count);
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
