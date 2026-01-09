/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023-2025 Red Hat Inc.
 *
 * Authors:
 * Hans de Goede <hdegoede@redhat.com>
 *
 * DebayerParams header
 */

#pragma once

#include <array>
#include <stdint.h>

#include "libcamera/internal/matrix.h"
#include "libcamera/internal/vector.h"

namespace libcamera {

struct DebayerParams {
	static constexpr unsigned int kRGBLookupSize = 256;

	struct CcmColumn {
		int16_t r;
		int16_t g;
		int16_t b;
	};

	using LookupTable = std::array<uint8_t, kRGBLookupSize>;
	using CcmLookupTable = std::array<CcmColumn, kRGBLookupSize>;

	using LSCLookupTable = uint8_t[16*16];

	/*
	 * Color lookup tables when CCM is not used.
	 *
	 * Each color of a debayered pixel is amended by the corresponding
	 * value in the given table.
	 */
	LookupTable red;
	LookupTable green;
	LookupTable blue;

	/*
	 * Color and gamma lookup tables when CCM is used.
	 *
	 * Each of the CcmLookupTable's corresponds to a CCM column; together they
	 * make a complete 3x3 CCM lookup table. The CCM is applied on debayered
	 * pixels and then the gamma lookup table is used to set the resulting
	 * values of all the three colors.
	 */
	CcmLookupTable redCcm;
	CcmLookupTable greenCcm;
	CcmLookupTable blueCcm;
	LookupTable gammaLut;

	LSCLookupTable LSC_red = {
		116, 90, 67, 44, 21, 5, 0, 0, 0, 0, 9, 29, 54, 75, 106, 136,
	99, 78, 55, 28, 6, 0, 0, 0, 0, 0, 1, 12, 39, 65, 91, 114,
	88, 70, 43, 14, 0, 0, 0, 0, 0, 0, 0, 3, 25, 55, 81, 100,
	81, 62, 31, 4, 0, 0, 0, 0, 0, 0, 0, 0, 13, 45, 73, 91,
	75, 54, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 35, 67, 86,
	71, 47, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 28, 62, 82,
	69, 42, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 57, 80,
	68, 39, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 55, 80,
	68, 39, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 55, 80,
	68, 40, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 57, 80,
	69, 44, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 28, 61, 81,
	72, 51, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 35, 66, 85,
	76, 57, 27, 2, 0, 0, 0, 0, 0, 0, 0, 0, 12, 43, 72, 90,
	83, 64, 37, 9, 0, 0, 0, 0, 0, 0, 0, 2, 23, 54, 79, 99,
	92, 70, 47, 21, 1, 0, 0, 0, 0, 0, 0, 10, 35, 63, 87, 113,
	107, 76, 54, 31, 9, 0, 0, 0, 0, 0, 5, 23, 46, 71, 100, 133
	   };
	LSCLookupTable LSC_green = {
		101, 75, 54, 34, 14, 2, 0, 0, 0, 0, 4, 20, 41, 61, 90, 119,
		84, 65, 45, 21, 3, 0, 0, 0, 0, 0, 0, 8, 29, 52, 76, 98,
		73, 57, 34, 10, 0, 0, 0, 0, 0, 0, 0, 1, 18, 44, 66, 84,
		65, 50, 24, 3, 0, 0, 0, 0, 0, 0, 0, 0, 9, 35, 59, 75,
		59, 42, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 27, 54, 70,
		55, 36, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 49, 66,
		53, 32, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 46, 65,
		52, 29, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 44, 65,
		52, 29, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 44, 65,
		52, 30, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 46, 65,
		54, 34, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 50, 66,
		56, 40, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 28, 55, 70,
		61, 46, 20, 1, 0, 0, 0, 0, 0, 0, 0, 0, 9, 36, 61, 76,
		68, 52, 30, 6, 0, 0, 0, 0, 0, 0, 0, 1, 18, 45, 67, 85,
		78, 59, 39, 16, 1, 0, 0, 0, 0, 0, 0, 7, 29, 53, 76, 99,
		93, 65, 45, 25, 6, 0, 0, 0, 0, 0, 3, 18, 38, 60, 89, 120
	   };
	LSCLookupTable LSC_blue = {
		233, 198, 170, 143, 115, 92, 75, 71, 71, 76, 95, 119, 148, 173, 209, 246,
		211, 185, 157, 124, 96, 77, 61, 55, 55, 63, 80, 100, 131, 161, 191, 219,
		196, 174, 142, 109, 81, 61, 47, 40, 40, 51, 63, 85, 116, 150, 178, 201,
		185, 164, 128, 94, 68, 47, 36, 27, 27, 40, 51, 73, 101, 138, 169, 190,
		178, 154, 116, 82, 56, 36, 26, 16, 16, 27, 41, 63, 90, 127, 161, 183,
		173, 145, 106, 72, 46, 26, 14, 7, 9, 16, 32, 54, 81, 118, 155, 178,
		170, 138, 98, 66, 40, 19, 5, 2, 5, 9, 26, 49, 76, 112, 150, 176,
		170, 135, 94, 63, 37, 16, 3, 0, 0, 6, 23, 46, 73, 109, 148, 176,
		170, 135, 94, 63, 37, 16, 3, 0, 0, 6, 23, 46, 73, 109, 148, 176,
		170, 138, 97, 65, 40, 19, 6, 3, 6, 9, 27, 50, 77, 113, 152, 176,
		172, 144, 104, 71, 46, 25, 14, 7, 9, 16, 34, 57, 84, 121, 158, 179,
		176, 152, 114, 79, 54, 34, 25, 16, 16, 27, 42, 66, 93, 131, 166, 185,
		183, 162, 126, 91, 64, 45, 34, 27, 27, 41, 53, 77, 106, 143, 174, 193,
		192, 172, 141, 106, 76, 60, 45, 41, 41, 53, 67, 91, 121, 157, 184, 206,
		207, 181, 155, 123, 92, 76, 60, 56, 56, 67, 82, 107, 139, 171, 197, 225,
		228, 193, 166, 139, 110, 91, 73, 73, 73, 82, 101, 126, 155, 185, 218, 255
	   };

	/*
	 * Per frame corrections as calculated by the IPA
	 */
	Matrix<float, 3, 3> ccm;
	RGB<float> blackLevel;
	float gamma;
	double contrastExp;
};

} /* namespace libcamera */
