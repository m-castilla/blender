/*
 * Copyright 2011-2013 Intel Corporation
 * Modifications Copyright 2014, Blender Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernel_util/COM_kernel_buffer.h"
#include "COM_Buffer.h"
#include "MEM_guardedalloc.h"

CCL_NAMESPACE_BEGIN

SimpleF4Buffer simple_buffer_to_f4(const SimpleBuffer &buffer)
{
  CCL::float4 *gausstab_f4 = (CCL::float4 *)MEM_mallocN(sizeof(float) * buffer.n_elems * 4,
                                                        __func__);
  for (int i = 0; i < buffer.n_elems; i++) {
    gausstab_f4[i] = CCL::make_float4_1(buffer.buffer[i]);
  }

  return {gausstab_f4, buffer.n_elems};
}

CCL_NAMESPACE_END
