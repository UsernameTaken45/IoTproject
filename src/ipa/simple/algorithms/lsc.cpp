#include "lsc.h"

#include <iostream>

#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>

#include <libcamera/control_ids.h>

#include "libcamera/internal/matrix.h"


namespace libcamera {

namespace ipa::soft::algorithms {

LOG_DEFINE_CATEGORY(IPASoftLsc)
int Lsc::init([[maybe_unused]] IPAContext &context, const YamlObject &tuningData)
{
	int ret_r = lsc_r.readYaml(tuningData["grids"], "ct", "r");
	int ret_g = lsc_r.readYaml(tuningData["grids"], "ct", "g");
	int ret_b = lsc_r.readYaml(tuningData["grids"], "ct", "b");

	if (ret_r < 0 || ret_g < 0 || ret_b < 0) {
		LOG(IPASoftLsc, Error)
			<< "Failed to parse 'lsc' parameter from tuning file.";
		return -1;
	}

	return 0;
}

int Lsc::configure([[maybe_unused]] IPAContext &context,
		   [[maybe_unused]] const IPAConfigInfo &configInfo)
{
	return 0;
}


void Lsc::prepare(IPAContext &context, [[maybe_unused]] const uint32_t frame,
		  [[maybe_unused]] IPAFrameContext &frameContext, [[maybe_unused]] DebayerParams *params)
{
	unsigned int ct = context.activeState.awb.temperatureK;
	if (ct == 0)
		ct = 2700;
	const Matrix<uint8_t, 16, 16> matrix_r = lsc_r.getInterpolated(ct);
	const Matrix<uint8_t, 16, 16> matrix_g = lsc_r.getInterpolated(ct);
	const Matrix<uint8_t, 16, 16> matrix_b = lsc_r.getInterpolated(ct);

	for (unsigned long i = 0;  i < matrix_r.data().size(); ++i) {
		params->LSC_red[i] = matrix_r.data()[i];
		params->LSC_green[i] = matrix_g.data()[i];
		params->LSC_blue[i] = matrix_b.data()[i];
	}
}

void Lsc::process([[maybe_unused]] IPAContext &context,
		  [[maybe_unused]] const uint32_t frame,
		  [[maybe_unused]] IPAFrameContext &frameContext,
		  [[maybe_unused]] const SwIspStats *stats,
		  [[maybe_unused]] ControlList &metadata)
{
}

REGISTER_IPA_ALGORITHM(Lsc, "Lsc")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */
