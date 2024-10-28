#pragma once

#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace cdda {
	using namespace shared;

	const auto SECTOR_LENGTH = size_t(cd::SECTOR_LENGTH);
	const auto FRAME_LENGTH = size_t(cd::PACKET_DATA_LENGTH);
	const auto SAMPLES_PER_SECTOR = size_t(SECTOR_LENGTH / sizeof(si16_t));
	const auto STEREO_CHANNELS = size_t(2);
	const auto STEREO_SAMPLE_LENGTH = size_t(STEREO_CHANNELS * sizeof(si16_t));
	const auto STEREO_SAMPLES_PER_SECTOR = size_t(SECTOR_LENGTH / STEREO_SAMPLE_LENGTH);
	const auto STEREO_SAMPLES_PER_FRAME = size_t(FRAME_LENGTH / STEREO_SAMPLE_LENGTH);

	#pragma pack(push, 1)

	// Mark members as volatile to prevent compiler from optimizing memory accesses when adhering to the strict aliasing rules.
	union Sample {
		volatile ui16_t ui;
		volatile si16_t si;
	};

	static_assert(sizeof(Sample) == 2);

	struct Sector {
		Sample samples[cdda::SAMPLES_PER_SECTOR];
	};

	static_assert(sizeof(Sector) == 2352);

	struct StereoSample {
		Sample l;
		Sample r;
	};

	static_assert(sizeof(StereoSample) == 4);

	struct StereoSector {
		StereoSample samples[STEREO_SAMPLES_PER_SECTOR];
	};

	static_assert(sizeof(StereoSector) == 2352);

	#pragma pack(pop)
}
}
