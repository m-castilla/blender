#include "COM_OpenCLManager.h"
#include "BLI_assert.h"
#include "COM_OpenCLDevice.h"
#include "COM_OpenCLPlatform.h"
#include "COM_defines.h"
#include "MEM_guardedalloc.h"
#include "clew.h"

static void CL_CALLBACK clContextError(const char *errinfo,
                                       const void * /*private_info*/,
                                       size_t /*cb*/,
                                       void * /*user_data*/)
{
  printf("OPENCL context error: %s\n", errinfo);
}

OpenCLManager::OpenCLManager()
{
}

void OpenCLManager::initialize()
{
  /* This will check for errors and skip if already initialized. */
  if (clewInit() != CLEW_SUCCESS) {
    return;
  }

  cl_int error;
  cl_uint numberOfPlatforms = 0;

  printIfError(clGetPlatformIDs(0, NULL, &numberOfPlatforms));

  cl_platform_id *platforms = (cl_platform_id *)MEM_mallocN(
      sizeof(cl_platform_id) * numberOfPlatforms, "OpenCLManager::initialize() -> platforms");
  printIfError(clGetPlatformIDs(numberOfPlatforms, platforms, 0));

  unsigned int indexPlatform;
  for (indexPlatform = 0; indexPlatform < numberOfPlatforms; indexPlatform++) {
    cl_platform_id platform = platforms[indexPlatform];
    cl_uint nDevices = 0;
    printIfError(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, 0, &nDevices));
    if (nDevices <= 0) {
      continue;
    }

    cl_device_id *devices = (cl_device_id *)MEM_mallocN(sizeof(cl_device_id) * nDevices,
                                                        "OpenCLManager::initialize() -> devices");
    printIfError(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, nDevices, devices, 0));

    /* Check devices has the necessary capabilies */
    auto result = checkDevicesRequiredCapabilities(devices, nDevices);
    MEM_freeN(devices);
    devices = nullptr;
    cl_device_id *capable_devices = result.first;
    nDevices = result.second;
    if (nDevices <= 0) {
      continue;
    }

    cl_context context = clCreateContext(
        NULL, nDevices, capable_devices, clContextError, NULL, &error);
    printIfError(error);

    auto source = loadKernelsSource();
    std::string source_code = source.first;
    std::string source_path = source.second;
    const char *program_source[2] = {source_code.c_str(), NULL};

    cl_program program = clCreateProgramWithSource(context, 1, program_source, 0, &error);

#if defined(DEBUG) && ENABLE_OPENCL_DEBUG
    auto build_options_debug = ("-g -s \"" + source_path + "\"");
    const char *build_options = build_options_debug.c_str();
#elif defined(COM_DEBUG) && ENABLE_OPENCL_DEBUG
    auto build_options_debug = ("-s \"" + source_path + "\"");
    const char *build_options = build_options_debug.c_str();
#else
    const char *build_options = "";
#endif
    error = clBuildProgram(program, nDevices, capable_devices, build_options, NULL, NULL);
    printIfError(error);
    if (error != CL_SUCCESS) {
      size_t ret_val_size = 0;

      for (int i = 0; i < nDevices; i++) {
        cl_device_id device = capable_devices[i];
        printIfError(
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size));

        char *build_log = (char *)MEM_mallocN(sizeof(char) * ret_val_size + 1,
                                              "OpenCLManager::initialize() -> build_log");
        printIfError(clGetProgramBuildInfo(
            program, device, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL));

        build_log[ret_val_size] = '\0';
        printf("Device %i build log: %s \n\n", i, build_log);
        MEM_freeN(build_log);
      }
    }
    else {
      OpenCLPlatform *platformObj = new OpenCLPlatform(*this, context, program);
      platformObj->initialize();
      m_platforms.push_back(platformObj);
      for (int i = 0; i < nDevices; i++) {
        cl_device_id device = capable_devices[i];

        OpenCLDevice *deviceObj = new OpenCLDevice(*this, *platformObj, device);
        m_devices.push_back(deviceObj);
      }
    }
    MEM_freeN(capable_devices);
  }
  MEM_freeN(platforms);

  ComputeManager::initialize();
}

std::vector<ComputeDevice *> OpenCLManager::getDevices()
{
  std::vector<ComputeDevice *> devices;
  for (auto dev : m_devices) {
    devices.push_back((ComputeDevice *)dev);
  }
  return devices;
}

void OpenCLManager::printIfError(int result_code)
{
  if (result_code != CL_SUCCESS) {
    printf("Compositor -> CLERROR[%d]: %s\n\n", result_code, clewErrorString(result_code));
  }
}

OpenCLManager::~OpenCLManager()
{
  if (isInitialized()) {
    OpenCLDevice *device;
    while (m_devices.size() > 0) {
      device = m_devices.back();
      m_devices.pop_back();
      delete device;
    }

    OpenCLPlatform *platform;
    while (m_platforms.size() > 0) {
      platform = m_platforms.back();
      m_platforms.pop_back();
      delete platform;
    }
  }
}

static bool checkDeviceVersion(cl_device_id device)
{
  const int req_major = 1, req_minor = 1;
  int major, minor;
  char version[256];
  clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, sizeof(version), &version, NULL);
  if (sscanf(version, "OpenCL C %d.%d", &major, &minor) < 2) {
    printf("OpenCL: failed to parse OpenCL C version string (%s).", version);
    return false;
  }

  if (!((major == req_major && minor >= req_minor) || (major > req_major))) {
    printf("OpenCL: C version 1.1 or later required, found %d.%d", major, minor);
    return false;
  }
  return true;
}

std::pair<cl_device_id *, int> OpenCLManager::checkDevicesRequiredCapabilities(
    cl_device_id *devices, int n_devices)
{
  cl_device_id *checked_devices = (cl_device_id *)MEM_mallocN(sizeof(cl_device_id) * n_devices,
                                                              __func__);

  int n_ok_devices = 0;
  for (int i = 0; i < n_devices; i++) {
    cl_device_id device = devices[i];

    cl_bool image_support = 0;
    printIfError(
        clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &image_support, NULL));
    if (!image_support) {
      continue;
    }

    cl_bool available = 0;
    printIfError(clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &available, NULL));

    if (!available) {
      continue;
    }

    if (!checkDeviceVersion(device)) {
      continue;
    }

    checked_devices[n_ok_devices] = device;
    n_ok_devices++;
  }
  return std::make_pair(checked_devices, n_ok_devices);
}
