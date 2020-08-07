/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2011, Blender Foundation.
 */

#include "COM_ConvertOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu_nocompat.h"
#include "IMB_colormanagement.h"
#include "immintrin.h"

using namespace std::placeholders;

ConvertBaseOperation::ConvertBaseOperation()
{
  this->m_inputOperation = NULL;
}

void ConvertBaseOperation::initExecution()
{
  this->m_inputOperation = this->getInputSocket(0)->getLinkedOp();
}

void ConvertBaseOperation::deinitExecution()
{
  this->m_inputOperation = NULL;
}

/* ******** Value to Color ******** */

ConvertValueToColorOperation::ConvertValueToColorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertValueToColorOp(CCL_WRITE(dst), CCL_READ(value))
{
  READ_DECL(value);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(value, dst_coords, value_pix);
  value_pix.y = value_pix.x;
  value_pix.z = value_pix.x;
  value_pix.w = 1.0f;
  WRITE_IMG(dst, dst_coords, value_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertValueToColorOperation::execPixels(ExecutionManager &man)
{
  auto value = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertValueToColorOp, _1, value);
  return computeWriteSeek(man, cpu_write, "convertValueToColorOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
  });
}

/* ******** Color to Value ******** */
ConvertColorToValueOperation::ConvertColorToValueOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::VALUE);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertColorToValueOp(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(color, dst_coords, color_pix);
  color_pix.x = (color_pix.x + color_pix.y + color_pix.z) / 3.0f;
  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertColorToValueOperation::execPixels(ExecutionManager &man)
{
  auto color = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertColorToValueOp, _1, color);
  return computeWriteSeek(man, cpu_write, "convertColorToValueOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
  });
}

/* ******** Color to BW ******** */
ConvertColorToBWOperation::ConvertColorToBWOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::VALUE);
}

void ConvertColorToBWOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    CPU_READ_DECL(src);
    CPU_WRITE_DECL(dst);
    CPU_LOOP_START(dst);
    CPU_WRITE_OFFSET(dst);
    CPU_READ_OFFSET(src, dst);

    dst_img.buffer[dst_offset] = IMB_colormanagement_get_luminance(src_img.buffer + src_offset);

    CPU_LOOP_END;
  };
  return cpuWriteSeek(man, cpuWrite);
}

/* ******** Color to Vector ******** */
ConvertColorToVectorOperation::ConvertColorToVectorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::VECTOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertColorToVectorOp(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(color, dst_coords, color_pix);
  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertColorToVectorOperation::execPixels(ExecutionManager &man)
{
  auto color = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertColorToVectorOp, _1, color);
  return computeWriteSeek(man, cpu_write, "convertColorToVectorOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
  });
}
/* ******** Value to Vector ******** */

ConvertValueToVectorOperation::ConvertValueToVectorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VECTOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertValueToVectorOp(CCL_WRITE(dst), CCL_READ(value))
{
  READ_DECL(value);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(value, dst_coords, value_pix);
  value_pix.y = value_pix.x;
  value_pix.z = value_pix.x;
  WRITE_IMG(dst, dst_coords, value_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertValueToVectorOperation::execPixels(ExecutionManager &man)
{
  auto value = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertValueToVectorOp, _1, value);
  return computeWriteSeek(man, cpu_write, "convertValueToVectorOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
  });
}

/* ******** Vector to Color ******** */

ConvertVectorToColorOperation::ConvertVectorToColorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::VECTOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertVectorToColorOp(CCL_WRITE(dst), CCL_READ(vector))
{
  READ_DECL(vector);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(vector, dst_coords, vector_pix);
  vector_pix.w = 1.0f;
  WRITE_IMG(dst, dst_coords, vector_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertVectorToColorOperation::execPixels(ExecutionManager &man)
{
  auto vector = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertVectorToColorOp, _1, vector);
  return computeWriteSeek(man, cpu_write, "convertVectorToColorOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*vector);
  });
}

/* ******** Vector to Value ******** */

ConvertVectorToValueOperation::ConvertVectorToValueOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::VECTOR);
  this->addOutputSocket(SocketType::VALUE);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertVectorToValueOp(CCL_WRITE(dst), CCL_READ(vector))
{
  READ_DECL(vector);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(vector, dst_coords, vector_pix);
  vector_pix.x = (vector_pix.x + vector_pix.y + vector_pix.z) / 3.0f;
  WRITE_IMG(dst, dst_coords, vector_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertVectorToValueOperation::execPixels(ExecutionManager &man)
{
  auto vector = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertVectorToValueOp, _1, vector);
  return computeWriteSeek(man, cpu_write, "convertVectorToValueOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*vector);
  });
}

/* ******** RGB to YCC ******** */

ConvertRGBToYCCOperation::ConvertRGBToYCCOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

void ConvertRGBToYCCOperation::setMode(int mode)
{
  switch (mode) {
    case 0:
      this->m_mode = BLI_YCC_ITU_BT601;
      break;
    case 2:
      this->m_mode = BLI_YCC_JFIF_0_255;
      break;
    case 1:
    default:
      this->m_mode = BLI_YCC_ITU_BT709;
      break;
  }
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertRgbToYccOp(CCL_WRITE(dst), CCL_READ(color), int mode)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(color, dst_coords, color_pix);
  color_pix = rgb_to_ycc(color_pix, mode);
  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertRGBToYCCOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertRgbToYccOp, _1, src, m_mode);
  return computeWriteSeek(man, cpu_write, "convertRgbToYccOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
    kernel->addIntArg(m_mode);
  });
}

void ConvertRGBToYCCOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_mode);
}

/* ******** YCC to RGB ******** */

ConvertYCCToRGBOperation::ConvertYCCToRGBOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

void ConvertYCCToRGBOperation::setMode(int mode)
{
  switch (mode) {
    case 0:
      this->m_mode = BLI_YCC_ITU_BT601;
      break;
    case 2:
      this->m_mode = BLI_YCC_JFIF_0_255;
      break;
    case 1:
    default:
      this->m_mode = BLI_YCC_ITU_BT709;
      break;
  }
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertYccToRgbOp(CCL_WRITE(dst), CCL_READ(ycc), int mode)
{
  READ_DECL(ycc);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(ycc, dst_coords, ycc_pix);
  ycc_pix = ycc_to_rgb(ycc_pix, mode);
  WRITE_IMG(dst, dst_coords, ycc_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertYCCToRGBOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertYccToRgbOp, _1, src, m_mode);
  return computeWriteSeek(man, cpu_write, "convertYccToRgbOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
    kernel->addIntArg(m_mode);
  });
}

void ConvertYCCToRGBOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_mode);
}

/* ******** RGB to YUV ******** */

ConvertRGBToYUVOperation::ConvertRGBToYUVOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertRgbToYuvOp(CCL_WRITE(dst), CCL_READ(rgb))
{
  READ_DECL(rgb);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(rgb, dst_coords, rgb_pix);
  rgb_pix = rgb_to_yuv(rgb_pix);
  WRITE_IMG(dst, dst_coords, rgb_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertRGBToYUVOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertRgbToYuvOp, _1, src);
  return computeWriteSeek(man, cpu_write, "convertRgbToYuvOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
  });
}

/* ******** YUV to RGB ******** */

ConvertYUVToRGBOperation::ConvertYUVToRGBOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertYuvToRgbOp(CCL_WRITE(dst), CCL_READ(yuv))
{
  READ_DECL(yuv);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(yuv, dst_coords, yuv_pix);
  yuv_pix = yuv_to_rgb(yuv_pix);
  WRITE_IMG(dst, dst_coords, yuv_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertYUVToRGBOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertYuvToRgbOp, _1, src);
  return computeWriteSeek(man, cpu_write, "convertYuvToRgbOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
  });
}

/* ******** RGB to HSV ******** */

ConvertRGBToHSVOperation::ConvertRGBToHSVOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertRgbToHsvOp(CCL_WRITE(dst), CCL_READ(rgb))
{
  READ_DECL(rgb);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(rgb, dst_coords, rgb_pix);
  rgb_pix = rgb_to_hsv(rgb_pix);
  WRITE_IMG(dst, dst_coords, rgb_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertRGBToHSVOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertRgbToHsvOp, _1, src);
  return computeWriteSeek(man, cpu_write, "convertRgbToHsvOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
  });
}

/* ******** HSV to RGB ******** */

ConvertHSVToRGBOperation::ConvertHSVToRGBOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertHsvToRgbOp(CCL_WRITE(dst), CCL_READ(hsv))
{
  READ_DECL(hsv);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(hsv, dst_coords, hsv_pix);
  hsv_pix = hsv_to_rgb(hsv_pix);
  WRITE_IMG(dst, dst_coords, hsv_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertHSVToRGBOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertHsvToRgbOp, _1, src);
  return computeWriteSeek(man, cpu_write, "convertHsvToRgbOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
  });
}

/* ******** Premul to Straight ******** */

ConvertPremulToStraightOperation::ConvertPremulToStraightOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertPremulToStraightOp(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(color, dst_coords, color_pix);
  color_pix = premul_to_straight(color_pix);
  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertPremulToStraightOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertPremulToStraightOp, _1, src);
  return computeWriteSeek(man, cpu_write, "convertPremulToStraightOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
  });
}

/* ******** Straight to Premul ******** */

ConvertStraightToPremulOperation::ConvertStraightToPremulOperation() : ConvertBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertStraightToPremulOp(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(color, dst_coords, color_pix);
  color_pix = straight_to_premul(color_pix);
  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertStraightToPremulOperation::execPixels(ExecutionManager &man)
{
  auto src = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::convertStraightToPremulOp, _1, src);
  return computeWriteSeek(man, cpu_write, "convertStraightToPremulOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*src);
  });
}

/* ******** Separate Channel ******** */
SeparateChannelOperation::SeparateChannelOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::VALUE);
  this->m_inputOperation = NULL;
}
void SeparateChannelOperation::initExecution()
{
  this->m_inputOperation = this->getInputSocket(0)->getLinkedOp();
}

void SeparateChannelOperation::deinitExecution()
{
  this->m_inputOperation = NULL;
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel separateChannel0Op(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);
  CPU_LOOP_START(dst);
  COORDS_TO_OFFSET(dst_coords);
  READ_IMG(color, dst_coords, color_pix);
  WRITE_IMG(dst, dst_coords, color_pix);
  CPU_LOOP_END
}

ccl_kernel separateChannel1Op(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);
  CPU_LOOP_START(dst);
  COORDS_TO_OFFSET(dst_coords);
  READ_IMG(color, dst_coords, color_pix);
  color_pix.x = color_pix.y;
  WRITE_IMG(dst, dst_coords, color_pix);
  CPU_LOOP_END
}

ccl_kernel separateChannel2Op(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);
  CPU_LOOP_START(dst);
  COORDS_TO_OFFSET(dst_coords);
  READ_IMG(color, dst_coords, color_pix);
  color_pix.x = color_pix.z;
  WRITE_IMG(dst, dst_coords, color_pix);
  CPU_LOOP_END
}

ccl_kernel separateChannel3Op(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);
  CPU_LOOP_START(dst);
  COORDS_TO_OFFSET(dst_coords);
  READ_IMG(color, dst_coords, color_pix);
  color_pix.x = color_pix.w;
  WRITE_IMG(dst, dst_coords, color_pix);
  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void SeparateChannelOperation::execPixels(ExecutionManager &man)
{
  auto color = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect & dst, std::shared_ptr<PixelsRect> color)> channel_func;
  std::string kernel_name = "";
  switch (m_channel) {
    case 0:
      channel_func = CCL_NAMESPACE::separateChannel0Op;
      kernel_name = "separateChannel0Op";
      break;
    case 1:
      channel_func = CCL_NAMESPACE::separateChannel1Op;
      kernel_name = "separateChannel1Op";
      break;
    case 2:
      channel_func = CCL_NAMESPACE::separateChannel2Op;
      kernel_name = "separateChannel2Op";
      break;
    case 3:
      channel_func = CCL_NAMESPACE::separateChannel3Op;
      kernel_name = "separateChannel3Op";
      break;
    default:
      BLI_assert(!"Non implemented channel index. Color is expected to have channels from 0 to 3");
      break;
  }
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      channel_func, _1, color);
  return computeWriteSeek(
      man, cpu_write, kernel_name, [&](ComputeKernel *kernel) { kernel->addReadImgArgs(*color); });
}

void SeparateChannelOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_channel);
}

/* ******** Combine Channels ******** */

CombineChannelsOperation::CombineChannelsOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
  this->setMainInputSocketIndex(0);
  this->m_inputChannel0Operation = NULL;
  this->m_inputChannel1Operation = NULL;
  this->m_inputChannel2Operation = NULL;
  this->m_inputChannel3Operation = NULL;
}

void CombineChannelsOperation::initExecution()
{
  this->m_inputChannel0Operation = this->getInputSocket(0)->getLinkedOp();
  this->m_inputChannel1Operation = this->getInputSocket(1)->getLinkedOp();
  this->m_inputChannel2Operation = this->getInputSocket(2)->getLinkedOp();
  this->m_inputChannel3Operation = this->getInputSocket(3)->getLinkedOp();
}

void CombineChannelsOperation::deinitExecution()
{
  this->m_inputChannel0Operation = NULL;
  this->m_inputChannel1Operation = NULL;
  this->m_inputChannel2Operation = NULL;
  this->m_inputChannel3Operation = NULL;
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel combineChannelsOp(
    CCL_WRITE(dst), CCL_READ(ch0), CCL_READ(ch1), CCL_READ(ch2), CCL_READ(ch3))
{
  READ_DECL(ch0);
  READ_DECL(ch1);
  READ_DECL(ch2);
  READ_DECL(ch3);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(ch0, dst_coords, ch0_pix);
  READ_IMG(ch1, dst_coords, ch1_pix);
  READ_IMG(ch2, dst_coords, ch2_pix);
  READ_IMG(ch3, dst_coords, ch3_pix);
  ch0_pix.y = ch1_pix.x;
  ch0_pix.z = ch2_pix.x;
  ch0_pix.w = ch3_pix.x;
  WRITE_IMG(dst, dst_coords, ch0_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void CombineChannelsOperation::execPixels(ExecutionManager &man)
{
  auto ch0 = m_inputChannel0Operation->getPixels(this, man);
  auto ch1 = m_inputChannel1Operation->getPixels(this, man);
  auto ch2 = m_inputChannel2Operation->getPixels(this, man);
  auto ch3 = m_inputChannel3Operation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::combineChannelsOp, _1, ch0, ch1, ch2, ch3);
  return computeWriteSeek(man, cpu_write, "combineChannelsOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*ch0);
    kernel->addReadImgArgs(*ch1);
    kernel->addReadImgArgs(*ch2);
    kernel->addReadImgArgs(*ch3);
  });
}
