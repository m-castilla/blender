#pragma once

#include "BLI_string_ref.hh"
#include "BLI_vector.hh"
#include "COM_ComputeKernel.h"
#include "clew.h"
#include <functional>

class OpenCLManager;
class OpenCLPlatform;
class OpenCLDevice;
class OpenCLKernel : public ComputeKernel {
 private:
  bool m_work_enqueued;
  OpenCLPlatform &m_platform;
  OpenCLManager &m_man;
  OpenCLDevice *m_device;
  blender::Vector<cl_mem> m_args_buffers;
  blender::Vector<void *> m_args_datas;

  cl_kernel m_cl_kernel;
  size_t m_max_group_size;
  size_t m_group_size_multiple;
  bool m_initialized;
  int m_args_count;
#ifdef DEBUG
  size_t m_local_mem_used;
  size_t m_private_mem_used;
#endif

 public:
  OpenCLKernel(OpenCLManager &man,
               OpenCLPlatform &platform,
               OpenCLDevice *device,
               const blender::StringRef kernel_name,
               cl_kernel cl_kernel);
  ~OpenCLKernel();

  void initialize();

  cl_kernel getClKernel()
  {
    return m_cl_kernel;
  }
  void reset(ComputeDevice *new_device) override;
  void clearArgs() override;
  void addBufferArg(void *cl_buffer) override;
  void addSamplerArg(ComputeInterpolation interp, ComputeExtend extend) override;
  void addBoolArg(bool value) override;
  void addIntArg(int value) override;
  void addInt3Arg(const int *value) override;
  void addFloatArg(float value) override;
  void addFloat2Arg(const float *value) override;
  void addFloat3Arg(const float *value) override;
  void addFloat4Arg(const float *value) override;

  void addFloat3ArrayArg(const float *float3_array,
                         int n_elems,
                         ComputeAccess mem_access) override;
  void addFloat4ArrayArg(const float *float4_array,
                         int n_elems,
                         ComputeAccess mem_access) override;
  void addIntArrayArg(int *int_array, int n_elems, ComputeAccess mem_access) override;
  void addFloatArrayArg(float *float_array, int n_elems, ComputeAccess mem_access) override;

  bool hasWorkEnqueued() override
  {
    return m_work_enqueued;
  }

  size_t getMaxGroupSize()
  {
    return m_max_group_size;
  }

  size_t getGroupSizeMultiple()
  {
    return m_group_size_multiple;
  }

 private:
  void addBufferArg(void *data, int elem_size, int n_elems, ComputeAccess mem_access);
  void addArg(size_t arg_size, const void *arg);

  template<typename TCl, typename TValue> void addPrimitiveArg(TValue value)
  {
    TCl cl_value = (TCl)value;
    addArg(sizeof(TCl), &cl_value);
  }

  template<typename TCl, typename TValue, const int N>
  void addPrimitiveNArg(const TValue *value_array)
  {
    TCl cl_value;
    for (int i = 0; i < N; i++) {
      cl_value.s[i] = value_array[i];
    }
    addArg(sizeof(TCl), &cl_value);
  }

  template<typename TCl, typename TValue, const int N>
  void addPrimitiveNArrayArg(const TValue *prim_array, int n_elems, ComputeAccess mem_access)
  {
    TCl *data = nullptr;
    size_t elem_size = sizeof(TCl);
    if (prim_array) {
      size_t data_size = elem_size * n_elems;
      data = (TCl *)MEM_mallocN(data_size, __func__);
      for (int elem_idx = 0; elem_idx < n_elems; elem_idx++) {
        const TValue *elem = &prim_array[elem_idx * N];
        TCl &cl_elem = data[elem_idx];
        for (int i = 0; i < N; i++) {
          cl_elem.s[i] = elem[i];
        }
      }
      m_args_datas.append(data);
    }
    addBufferArg(data, elem_size, n_elems, mem_access);
  }
};
