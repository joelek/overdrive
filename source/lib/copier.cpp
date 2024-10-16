#include "copier.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include "cdda.h"
#include "exceptions.h"
#include "idiv.h"
#include "iso9660.h"
#include "string.h"

namespace overdrive {
namespace copier {
	auto ExtractedSector::has_identical_sector_data(
		const ExtractedSector& that
	) -> bool_t {
		return std::memcmp(this->sector_data, that.sector_data, sizeof(sector_data)) == 0;
	}

	auto ExtractedSector::has_identical_subchannels_data(
		const ExtractedSector& that
	) -> bool_t {
		return std::memcmp(this->subchannels_data, that.subchannels_data, sizeof(subchannels_data)) == 0;
	}

	auto ExtractedSector::has_identical_c2_data(
		const ExtractedSector& that
	) -> bool_t {
		return std::memcmp(this->c2_data, that.c2_data, sizeof(c2_data)) == 0;
	}

	auto get_number_of_identical_copies(
		std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector
	) -> size_t {
		auto counters = std::vector<size_t>(1);
		for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
			auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
			if (extracted_sectors.size() > 0) {
				// Sort in decreasing order.
				std::sort(extracted_sectors.begin(), extracted_sectors.end(), [](const ExtractedSector& one, const ExtractedSector& two) -> bool_t {
					return two.counter < one.counter;
				});
				auto& extracted_sector = extracted_sectors.at(0);
				counters.resize(std::max(extracted_sector.counter + 1, counters.size()));
				counters.at(extracted_sector.counter) += 1;
			} else {
				counters.at(0) += 1;
			}
		}
		for (auto counter_index = size_t(0); counter_index < counters.size(); counter_index += 1) {
			auto counter = counters.at(counter_index);
			if (counter != 0) {
				return counter_index;
			}
		}
		return 0;
	}

	auto get_bad_sector_indices(
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector,
		size_t first_sector
	) -> std::vector<size_t> {
		auto bad_sector_indices = std::vector<size_t>();
		for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
			auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
			if (extracted_sectors.size() == 0) {
				bad_sector_indices.push_back(first_sector + sector_index);
			} else {
				auto& extracted_sector = extracted_sectors.at(0);
				if (extracted_sector.counter == 0) {
					bad_sector_indices.push_back(first_sector + sector_index);
				}
			}
		}
		return bad_sector_indices;
	}

	auto get_bad_sector_indices_per_path(
		const drive::Drive& drive,
		size_t user_data_offset,
		size_t user_data_length,
		const std::vector<size_t>& bad_sector_indices
	) -> std::optional<std::map<std::string, std::vector<size_t>>> {
		try {
			if (user_data_length == iso9660::USER_DATA_SIZE) {
				auto sector = ExtractedSector();
				auto fs = iso9660::FileSystem([&](size_t sector_index, void* user_data) -> void {
					drive.read_absolute_sector(cd::get_absolute_sector_index(sector_index), &sector.sector_data, nullptr, nullptr);
					std::memcpy(user_data, sector.sector_data + user_data_offset, iso9660::USER_DATA_SIZE);
				});
				auto bad_sector_indices_per_path = std::map<std::string, std::vector<size_t>>();
				for (auto bad_sector_index : bad_sector_indices) {
					auto optional_path = fs.get_path(cd::get_relative_sector_index(bad_sector_index));
					if (optional_path) {
						auto path = std::string("/") + string::join(optional_path.value(), "/");
						auto& bad_sector_indices = bad_sector_indices_per_path[path];
						bad_sector_indices.push_back(bad_sector_index);
					} else {
						auto& bad_sector_indices = bad_sector_indices_per_path[""];
						bad_sector_indices.push_back(bad_sector_index);
					}
				}
				return bad_sector_indices_per_path;
			}
		} catch (const exceptions::SCSIException& e) {
			fprintf(stderr, "%s\n", std::format("Error reading ISO 9660 file system!").c_str());
		}
		return std::optional<std::map<std::string, std::vector<size_t>>>();
	}

	auto read_absolute_sector_range(
		const drive::Drive& drive,
		size_t first_sector,
		size_t last_sector,
		size_t min_passes,
		size_t max_passes,
		size_t max_retries,
		size_t min_copies,
		size_t max_copies
	) -> std::vector<std::vector<ExtractedSector>> {
		auto length_sectors = last_sector - first_sector;
		auto extracted_sectors_vector = std::vector<std::vector<ExtractedSector>>(length_sectors);
		drive.set_read_retry_count(max_retries);
		for (auto pass_index = size_t(0); pass_index < max_passes; pass_index += 1) {
			fprintf(stderr, "%s\n", std::format("Running pass {}", pass_index).c_str());
			for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
				auto sector = ExtractedSector();
				auto success = false;
				try {
					drive.read_absolute_sector(sector_index, &sector.sector_data, &sector.subchannels_data, &sector.c2_data);
					success = true;
				} catch (const exceptions::SCSIException& e) {
					fprintf(stderr, "%s\n", std::format("Error reading sector {}!", sector_index).c_str());
				}
				auto& extracted_sectors = extracted_sectors_vector.at(sector_index - first_sector);
				auto extracted_sector_with_matching_data = pointer<ExtractedSector>(nullptr);
				for (auto& extracted_sector : extracted_sectors) {
					if (extracted_sector.has_identical_sector_data(sector)) {
						extracted_sector_with_matching_data = &extracted_sector;
						break;
					}
				}
				if (!extracted_sector_with_matching_data) {
					extracted_sectors.push_back(std::move(sector));
					extracted_sector_with_matching_data = &extracted_sectors.back();
				}
				if (success) {
					extracted_sector_with_matching_data->counter += 1;
				}
			}
			auto number_of_identical_copies = get_number_of_identical_copies(extracted_sectors_vector);
			fprintf(stderr, "%s\n", std::format("Got {} identical copies", number_of_identical_copies).c_str());
			if (pass_index + 1 >= min_passes && number_of_identical_copies >= max_copies) {
				break;
			}
		}
		auto number_of_identical_copies = get_number_of_identical_copies(extracted_sectors_vector);
		if (number_of_identical_copies < min_copies) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("number of identical copies", number_of_identical_copies, min_copies, max_copies));
		}
		return extracted_sectors_vector;
	}

	auto read_absolute_sector_range_with_correction(
		const drive::Drive& drive,
		size_t first_sector,
		size_t last_sector,
		size_t min_passes,
		size_t max_passes,
		size_t max_retries,
		size_t min_copies,
		size_t max_copies,
		si_t read_correction_samples
	) -> std::vector<std::vector<ExtractedSector>> {
		fprintf(stderr, "%s\n", std::format("Using read correction [samples]: {}", read_correction_samples).c_str());
		auto read_correction_bytes = read_correction_samples * si_t(cdda::STEREO_SAMPLE_LENGTH);
		fprintf(stderr, "%s\n", std::format("Using read correction [bytes]: {}", read_correction_bytes).c_str());
		auto start_offset_bytes = si_t(first_sector * cd::SECTOR_LENGTH) + read_correction_bytes;
		auto end_offset_bytes = si_t(last_sector * cd::SECTOR_LENGTH) + read_correction_bytes;
		auto adjusted_first_sector = idiv::floor(start_offset_bytes, cd::SECTOR_LENGTH);
		auto adjusted_last_sector = idiv::ceil(end_offset_bytes, cd::SECTOR_LENGTH);
		auto prefix_length = read_correction_bytes - ((adjusted_first_sector - first_sector) * cd::SECTOR_LENGTH);
		auto suffix_length = cd::SECTOR_LENGTH - prefix_length;
		if (read_correction_bytes != 0) {
			fprintf(stderr, "%s\n", std::format("Adjusted sector range is from {} to {}", adjusted_first_sector, adjusted_last_sector).c_str());
			fprintf(stderr, "%s\n", std::format("The first {} bytes of sector data will be discarded", prefix_length).c_str());
			fprintf(stderr, "%s\n", std::format("The last {} bytes of sector data will be discarded", suffix_length).c_str());
		}
		auto extracted_sectors_vector = copier::read_absolute_sector_range(
			drive,
			adjusted_first_sector,
			adjusted_last_sector,
			min_passes,
			max_passes,
			max_retries,
			min_copies,
			max_copies
		);
		if (read_correction_bytes != 0) {
			for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
				auto& extracted_sectors = extracted_sectors_vector.at(sector_index - first_sector);
				auto& extracted_sector = extracted_sectors.at(0);
				std::memmove(&extracted_sector.sector_data[0], &extracted_sector.sector_data[prefix_length], suffix_length);
				auto& next_extracted_sectors = extracted_sectors_vector.at(sector_index - first_sector + 1);
				auto& next_extracted_sector = next_extracted_sectors.at(0);
				std::memmove(&extracted_sector.sector_data[suffix_length], &next_extracted_sector.sector_data[0], prefix_length);
			}
			extracted_sectors_vector.resize(last_sector - first_sector); // This hides read errors in last sector.
		}
		return extracted_sectors_vector;
	}

	auto write_sector_data_to_file(
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector,
		const std::string& path,
		size_t sector_data_offset,
		size_t sector_data_length
	) -> void {
		auto handle = std::fopen(path.c_str(), "wb+");
		if (handle == nullptr) {
			OVERDRIVE_THROW(exceptions::IOOpenException(path));
		}
		try {
			fprintf(stderr, "%s\n", std::format("Saving track sector data to \"{}\"", path).c_str());
			for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
				auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
				auto& extracted_sector = extracted_sectors.at(0);
				if (std::fwrite(extracted_sector.sector_data + sector_data_offset, sector_data_length, 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
			}
		} catch (...) {
			std::fclose(handle);
			throw;
		}
	}

	auto log_bad_sector_indices(
		const drive::Drive& drive,
		const disc::TrackInfo& track,
		const std::vector<size_t>& bad_sector_indices
	) -> void {
		if (disc::is_data_track(track.type)) {
			auto user_data_offset = disc::get_user_data_offset(track.type);
			auto user_data_length = disc::get_user_data_length(track.type);
			auto bad_sector_indices_per_path = copier::get_bad_sector_indices_per_path(drive, user_data_offset, user_data_length, bad_sector_indices);
			if (bad_sector_indices_per_path) {
				for (auto& entry : bad_sector_indices_per_path.value()) {
					fprintf(stderr, "%s\n", std::format("File at path \"{}\" contains {} bad sectors!", std::filesystem::path(entry.first).string(), entry.second.size()).c_str());
				}
			} else {
				fprintf(stderr, "%s\n", std::format("Track number {} containing data has {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
			}
		} else {
			fprintf(stderr, "%s\n", std::format("Track number {} containing audio has {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
		}
	}
}
}
