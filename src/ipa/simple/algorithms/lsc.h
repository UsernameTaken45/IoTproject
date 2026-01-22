/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024-2025, Red Hat Inc.
 *
 * Color correction matrix
 */

#pragma once

#include <optional>

#include "libcamera/internal/matrix.h"

#include <libipa/interpolator.h>

#include "algorithm.h"

namespace libcamera {

namespace ipa::soft::algorithms {


class Lsc : public Algorithm
{
public:
	Lsc() = default;
	~Lsc() = default;

	int init(IPAContext &context, const YamlObject &tuningData) override;
	int configure(IPAContext &context,
		      const IPAConfigInfo &configInfo) override;
	void prepare(IPAContext &context,
		     const uint32_t frame,
		     IPAFrameContext &frameContext,
		     DebayerParams *params) override;
	void process(IPAContext &context, const uint32_t frame,
		     IPAFrameContext &frameContext,
		     const SwIspStats *stats,
		     ControlList &metadata) override;

private:
	Interpolator<Matrix<uint8_t, 16, 16>> lsc_r;
	Interpolator<Matrix<uint8_t, 16, 16>> lsc_g;
	Interpolator<Matrix<uint8_t, 16, 16>> lsc_b;
};

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */
