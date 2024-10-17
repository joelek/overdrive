#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include "cd.h"
#include "disc.h"
#include "drive.h"
#include "options.h"
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
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector,
		size_t first_sector
	) -> std::vector<size_t>;

	auto get_bad_sector_indices_per_path(
		const drive::Drive& drive,
		size_t user_data_offset,
		size_t user_data_length,
		const std::vector<size_t>& bad_sector_indices
	) -> std::optional<std::map<std::string, std::vector<size_t>>>;

	auto read_absolute_sector_range(
		const drive::Drive& drive,
		size_t first_sector,
		size_t last_sector,
		size_t min_passes,
		size_t max_passes,
		size_t max_retries,
		size_t min_copies,
		size_t max_copies
	) -> std::vector<std::vector<ExtractedSector>>;

	auto read_track(
		const drive::Drive& drive,
		const disc::TrackInfo& track,
		const options::Options& options
	) -> std::vector<std::vector<ExtractedSector>>;

	auto append_sector_data(
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector,
		const std::string& path,
		size_t sector_data_offset,
		size_t sector_data_length,
		std::FILE* handle
	) -> void;

	auto open_handle(
		const std::string& path
	) -> std::FILE*;

	auto close_handle(
		std::FILE* handle
	) -> void;

	auto write_sector_data_to_file(
		const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector,
		const std::string& path,
		size_t sector_data_offset,
		size_t sector_data_length
	) -> void;

	auto log_bad_sector_indices(
		const drive::Drive& drive,
		const disc::TrackInfo& track,
		const std::vector<size_t>& bad_sector_indices
	) -> void;
}
}
