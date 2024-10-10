#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include "cd.h"
#include "drive.h"
#include "shared.h"

namespace overdrive {
namespace copier {
	using namespace shared;

	class ExtractedSector {
		public:

		byte_t sector_data[cd::SECTOR_LENGTH];
		byte_t subchannels_data[cd::SUBCHANNELS_LENGTH];
		byte_t c2_data[cd::C2_LENGTH];
		size_t counter;

		auto has_identical_sector_data(
			const ExtractedSector& that
		) -> bool_t;

		auto has_identical_subchannels_data(
			const ExtractedSector& that
		) -> bool_t;

		auto has_identical_c2_data(
			const ExtractedSector& that
		) -> bool_t;

		protected:
	};

	auto get_number_of_identical_copies(
		std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector
	) -> size_t;

	auto get_bad_sector_indices(
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector
	) -> std::vector<size_t>;

	auto get_bad_sector_indices_per_path(
		const drive::Drive& drive,
		size_t user_data_offset,
		size_t user_data_length,
		const std::vector<size_t>& bad_sector_indices
	) -> std::optional<std::map<std::string, std::vector<size_t>>>;

	auto read_sector_range(
		const drive::Drive& drive,
		size_t first_sector,
		size_t last_sector,
		size_t min_passes,
		size_t max_passes,
		size_t max_read_reties,
		size_t min_copies,
		size_t max_copies
	) -> std::vector<std::vector<ExtractedSector>>;
}
}
