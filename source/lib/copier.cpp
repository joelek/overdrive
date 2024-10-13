#include "copier.h"

#include <algorithm>
#include <cstring>
#include <format>
#include "exceptions.h"
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
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector
	) -> std::vector<size_t> {
		auto bad_sector_indices = std::vector<size_t>();
		for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
			auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
			if (extracted_sectors.size() == 0) {
				bad_sector_indices.push_back(sector_index);
			} else {
				auto& extracted_sector = extracted_sectors.at(0);
				if (extracted_sector.counter == 0) {
					bad_sector_indices.push_back(sector_index);
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
					drive.read_sector(sector_index, &sector.sector_data, nullptr, nullptr);
					std::memcpy(user_data, sector.sector_data + user_data_offset, iso9660::USER_DATA_SIZE);
				});
				auto bad_sector_indices_per_path = std::map<std::string, std::vector<size_t>>();
				for (auto bad_sector_index : bad_sector_indices) {
					auto optional_path = fs.get_path(bad_sector_index);
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

	auto read_sector_range(
		const drive::Drive& drive,
		size_t first_sector,
		size_t last_sector,
		size_t min_passes,
		size_t max_passes,
		size_t max_read_reties,
		size_t min_copies,
		size_t max_copies
	) -> std::vector<std::vector<ExtractedSector>> {
		auto length_sectors = last_sector - first_sector;
		auto extracted_sectors_vector = std::vector<std::vector<ExtractedSector>>(length_sectors);
		drive.set_read_retry_count(max_read_reties);
		for (auto pass_index = size_t(0); pass_index < max_passes; pass_index += 1) {
			fprintf(stderr, "%s\n", std::format("Running pass {}", pass_index).c_str());
			for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
				auto sector = ExtractedSector();
				auto success = false;
				try {
					drive.read_sector(sector_index, &sector.sector_data, &sector.subchannels_data, &sector.c2_data);
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
}
}
