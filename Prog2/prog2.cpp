#include <iostream>
#include <fstream>
#include <vector>
#include <link.h>
#include <dlfcn.h>
#include "neuron/api/NeuronAdapter.h"


void* load_func(void* handle, const char* func_name) {
    void* func_ptr = dlsym(handle, func_name);
    if (func_ptr == nullptr) {
        std::cerr << "Find " << func_name << " function failed." << std::endl;
        exit(2);
    }
    return func_ptr;
}

int main(int ac, char *av[])
{
    void* handle;
    typedef int (*Neuron_getVersion)(NeuronRuntimeVersion *version);
    typedef int (*Neuron_getDeviceCount)(uint32_t *numDevices);
    typedef int (*Neuron_getDevice)(uint32_t devindex, NeuronDevice **device);
    typedef int (*NeuronDevice_getName)(const NeuronDevice *device, const char **name);
    typedef int (*NeuronDevice_getDescription)(const NeuronDevice *device, const char **description);

    handle = dlopen("libneuronusdk_adapter.mtk.so", RTLD_LAZY);
    if (handle == nullptr) {
        std::cerr << dlerror() << std::endl;
        std::cerr << "Failed to open libneuronusdk_adapter.mtk.so" << std::endl;
        exit(2);
    }

#define LOAD_FUNCTION(FUNC_NAME, VARIABLE_NAME)                         \
    FUNC_NAME VARIABLE_NAME = reinterpret_cast<FUNC_NAME>(load_func(handle, #FUNC_NAME))

    LOAD_FUNCTION(Neuron_getVersion, neuron_getVersion);
    LOAD_FUNCTION(Neuron_getDeviceCount, neuron_getDeviceCount);
    LOAD_FUNCTION(Neuron_getDevice, neuron_getDevice);
    LOAD_FUNCTION(NeuronDevice_getName, neurondevice_getName);
    LOAD_FUNCTION(NeuronDevice_getDescription, neurondevice_getDescription);
#undef LOAD_FUNCTION
    NeuronRuntimeVersion version;
    neuron_getVersion(&version);
    std::cout << "Neuron version : "
              << static_cast<uint32_t>(version.major) << "."
              << static_cast<uint32_t>(version.minor) << "."
              << static_cast<uint32_t>(version.patch) << std::endl;

    uint32_t num_devices;
    neuron_getDeviceCount(&num_devices);
    std::cout << "Number of devices " << num_devices << std::endl;
    for(int i=0; i< num_devices; i++) {
        NeuronDevice *devicep;
        const char *namep;
        const char *desc;
        neuron_getDevice(i, &devicep);
        neurondevice_getName(devicep, &namep);
        neurondevice_getDescription(devicep, &desc);
        std::cout << "\t" << i << ":" << namep << " - " << desc << std::endl;
    }
    dlclose(handle);
    return 0;
}
