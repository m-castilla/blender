#ifndef __COM_KERNEL_ALGO_H__
#define __COM_KERNEL_ALGO_H__

#include "kernel_util/COM_kernel_color.h"
#include "kernel_util/COM_kernel_geom.h"
#include "kernel_util/COM_kernel_math.h"

CCL_NAMESPACE_BEGIN

ccl_device_inline float finv_test(const float f, const bool test)
{
  return (LIKELY(test == false)) ? f : 1.0f - f;
}

/* BokehImageOperation */

/**
 * \brief determine the coordinate of a flap corner.
 *
 * \param r: result in bokehimage space are stored [x,y]
 * \param flapNumber: the flap number to calculate
 * \param distance: the lens distance is used to simulate lens shifts
 */
ccl_device_inline float2 bokehStartPointOfFlap(
    int flapNumber, float distance, float2 center, float flap_rad, float flap_rad_add)
{
  return make_float2(sinf(flap_rad * flapNumber + flap_rad_add) * distance + center.x,
                     cosf(flap_rad * flapNumber + flap_rad_add) * distance + center.y);
}

/**
 * \brief Determine if a coordinate is inside the bokeh image
 *
 * \param distance: the distance that will be used.
 * This parameter is modified a bit to mimic lens shifts.
 * \param x: the x coordinate of the pixel to evaluate
 * \param y: the y coordinate of the pixel to evaluate
 * \return float range 0..1 0 is completely outside
 */
ccl_device_inline float bokehIsInside(const float c_distance,
                                      const float2 coord,
                                      const float2 center,
                                      const float flap_rad,
                                      const float flap_rad_add,
                                      const float rounding,
                                      const float catadioptric)
{
  float insideBokeh = 0.0f;
  float2 delta = coord - center;

  const float distanceToCenter = distance(coord, center);
  const float bearing = (atan2f(delta.x, delta.y) + (M_PI_F * 2.0f));
  int flapNumber = (int)((bearing - flap_rad_add) / flap_rad);

  float2 lineP1 = bokehStartPointOfFlap(flapNumber, c_distance, center, flap_rad, flap_rad_add);
  float2 lineP2 = bokehStartPointOfFlap(
      flapNumber + 1, c_distance, center, flap_rad, flap_rad_add);
  float2 closestPoint = closest_to_line_v2(coord, lineP1, lineP2);

  const float distanceLineToCenter = distance(center, closestPoint);
  const float distanceRoundingToCenter = (1.0f - rounding) * distanceLineToCenter +
                                         rounding * c_distance;

  const float catadioptricDistanceToCenter = distanceRoundingToCenter * catadioptric;
  if (distanceRoundingToCenter >= distanceToCenter &&
      catadioptricDistanceToCenter <= distanceToCenter) {
    if (distanceRoundingToCenter - distanceToCenter < 1.0f) {
      insideBokeh = (distanceRoundingToCenter - distanceToCenter);
    }
    else if (catadioptric != 0.0f && distanceToCenter - catadioptricDistanceToCenter < 1.0f) {
      insideBokeh = (distanceToCenter - catadioptricDistanceToCenter);
    }
    else {
      insideBokeh = 1.0f;
    }
  }
  return insideBokeh;
}

/* END of BokehImageOperation */

/* ColorBalanceASCCDLOperation */
ccl_device_inline float4 colorbalance_cdl(const float4 in,
                                          const float4 offset,
                                          const float4 power,
                                          const float4 slope)
{
  float4 res = in * slope + offset;

  /* prevent NaN */
  float4 zero4 = make_float4_1(0.0f);
  res = select(res, zero4, res < zero4);

  res.x = powf(res.x, power.x);
  res.y = powf(res.y, power.y);
  res.z = powf(res.z, power.z);
  res.w = in.w;

  return res;
}

/* END of ColorBalanceASCCDLOperation */

/* ColorBalanceLGGOperation */

ccl_device_inline float4 colorbalance_lgg(float4 in,
                                          float4 lift_lgg,
                                          float4 gamma_inv,
                                          float4 gain)
{
  /* 1:1 match with the sequencer with linear/srgb conversions, the conversion isnt pretty
   * but best keep it this way, since testing for durian shows a similar calculation
   * without lin/srgb conversions gives bad results (over-saturated shadows) with colors
   * slightly below 1.0. some correction can be done but it ends up looking bad for shadows or
   * lighter tones - campbell */
  float4 res;
  res.x = linearrgb_to_srgb(in.x);
  res.y = linearrgb_to_srgb(in.y);
  res.z = linearrgb_to_srgb(in.z);
  res = (((res - 1.0f) * lift_lgg) + 1.0f) * gain;
  if (res.x < 0.0f) {
    res.x = 0.0f;
  }
  if (res.y < 0.0f) {
    res.y = 0.0f;
  }
  if (res.z < 0.0f) {
    res.z = 0.0f;
  }
  res.x = powf(srgb_to_linearrgb(res.x), gamma_inv.x);
  res.y = powf(srgb_to_linearrgb(res.y), gamma_inv.y);
  res.z = powf(srgb_to_linearrgb(res.z), gamma_inv.z);
  res.w = in.w;

  return res;
}
/* END of ColorBalanceLGGOperation */

CCL_NAMESPACE_END

#endif
