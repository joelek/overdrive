#pragma once

#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace cdda {
	using namespace shared;

	const auto SECTOR_LENGTH = size_t(cd::SECTOR_LENGTH);
	const auto FRAME_LENGTH = size_t(cd::PACKET_DATA_LENGTH);
	const auto STEREO_CHANNELS = size_t(2);
	const auto STEREO_SAMPLE_LENGTH = size_t(STEREO_CHANNELS * sizeof(si16_t));
	const auto STEREO_SAMPLES_PER_SECTOR = size_t(SECTOR_LENGTH / STEREO_SAMPLE_LENGTH);
	const auto STEREO_SAMPLES_PER_FRAME = size_t(FRAME_LENGTH / STEREO_SAMPLE_LENGTH);

	#pragma pack(push, 1)

	struct StereoSector {
		si16_t samples[STEREO_SAMPLES_PER_SECTOR][STEREO_CHANNELS];
	};

	static_assert(sizeof(StereoSector) == 2352);

	union Sector {
		StereoSector stereo;
	};

	static_assert(sizeof(Sector) == 2352);

	#pragma pack(pop)
}
}
