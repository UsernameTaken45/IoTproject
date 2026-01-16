#include "lsc.h"

#include <iostream>

#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>

#include <libcamera/control_ids.h>

#include "libcamera/internal/matrix.h"

namespace {

constexpr unsigned int kTemperatureThreshold = 100;

}

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

int Lsc::configure(IPAContext &context,
		   [[maybe_unused]] const IPAConfigInfo &configInfo)
{
	context.activeState.knobs.saturation = std::optional<double>();

	return 0;
}

void Lsc::queueRequest(typename Module::Context &context,
		       [[maybe_unused]] const uint32_t frame,
		       [[maybe_unused]] typename Module::FrameContext &frameContext,
		       const ControlList &controls)
{
	const auto &saturation = controls.get(controls::Saturation);
	if (saturation.has_value()) {
		context.activeState.knobs.saturation = saturation;
		LOG(IPASoftLsc, Debug) << "Setting saturation to " << saturation.value();
	}
}

void Lsc::applySaturation(Matrix<float, 3, 3> &ccm, float saturation)
{
	/* https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion */
	const Matrix<float, 3, 3> rgb2ycbcr{
		{ 0.256788235294, 0.504129411765, 0.0979058823529,
		  -0.148223529412, -0.290992156863, 0.439215686275,
		  0.439215686275, -0.367788235294, -0.0714274509804 }
	};
	const Matrix<float, 3, 3> ycbcr2rgb{
		{ 1.16438356164, 0, 1.59602678571,
		  1.16438356164, -0.391762290094, -0.812967647235,
		  1.16438356164, 2.01723214285, 0 }
	};
	const Matrix<float, 3, 3> saturationMatrix{
		{ 1, 0, 0,
		  0, saturation, 0,
		  0, 0, saturation }
	};
	ccm = ycbcr2rgb * saturationMatrix * rgb2ycbcr * ccm;
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
		  IPAFrameContext &frameContext,
		  [[maybe_unused]] const SwIspStats *stats,
		  ControlList &metadata)
{
	metadata.set(controls::ColourCorrectionMatrix, frameContext.ccm.ccm.data());

	const auto &saturation = frameContext.saturation;
	metadata.set(controls::Saturation, saturation.value_or(1.0));
}

REGISTER_IPA_ALGORITHM(Lsc, "Lsc")

} /* namespace ipa::soft::algorithms */

} /* namespace libcamera */
