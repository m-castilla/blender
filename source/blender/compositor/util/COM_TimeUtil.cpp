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
 * Copyright 2020, Blender Foundation.
 */

#include "COM_TimeUtil.h"
#include "BLI_assert.h"
#include <string>

using namespace std::chrono;
namespace TimeUtil {

uint64_t getNowMsTime()
{
  auto now = steady_clock::now();
  auto duration = now.time_since_epoch();
  return durationToMs(duration);
}

uint64_t getNowNsTime()
{
  auto now = steady_clock::now();
  auto duration = now.time_since_epoch();
  return durationToNs(duration);
}

uint64_t durationToMs(steady_clock::duration duration)
{
  return (uint64_t)duration_cast<milliseconds>(duration).count();
}

uint64_t durationToNs(steady_clock::duration duration)
{
  return (uint64_t)duration_cast<nanoseconds>(duration).count();
}

time_point<steady_clock, nanoseconds> nsEpochToTimePoint(uint64_t epoch_time_nanoseconds)
{
  return time_point<steady_clock, nanoseconds>(nanoseconds(epoch_time_nanoseconds));
}

}  // namespace TimeUtil