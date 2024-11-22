#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include "neuron/api/NeuronAdapter.h"

void* load_func(void* handle, const char* func_name) {
    /* Load the function specified by func_name, and exit if the loading is
     * failed. */
    void* func_ptr = dlsym(handle, func_name);
    if (func_ptr == nullptr) {
        std::cerr << "Find " << func_name << " function failed." << std::endl;
        exit(2);
    }
    return func_ptr;
}

int main()
{
    void* handle;
    // typedef to the build functions pointer signatures
    typedef int (*Neuron_getVersion)(NeuronRuntimeVersion * version);
    typedef int (*NeuronModel_create)(NeuronModel * *model);
    typedef void (*NeuronModel_free)(NeuronModel * model);
    typedef int (*NeuronModel_finish)(NeuronModel * model);
    typedef int (*NeuronModel_addOperand)(NeuronModel * model, const NeuronOperandType* type);
    typedef int (*NeuronModel_setOperandValue)(NeuronModel * model, int32_t index,
                                               const void* buffer, size_t length);
    typedef int (*NeuronModel_addOperation)(NeuronModel * model, NeuronOperationType type,
                                            uint32_t inputCount, const uint32_t* inputs,
                                            uint32_t outputCount, const uint32_t* outputs);
    typedef int (*NeuronModel_identifyInputsAndOutputs)(NeuronModel * model, uint32_t inputCount,
                                                        const uint32_t* inputs, uint32_t outputCount,
                                                        const uint32_t* outputs);
    typedef int (*NeuronCompilation_create)(NeuronModel * model, NeuronCompilation * *compilation);
    typedef void (*NeuronCompilation_free)(NeuronCompilation * compilation);
    typedef int (*NeuronCompilation_finish)(NeuronCompilation * compilation);
    typedef int (*NeuronExecution_create)(NeuronCompilation * compilation,
                                          NeuronExecution * *execution);
    typedef void (*NeuronExecution_free)(NeuronExecution * execution);
    typedef int (*NeuronExecution_setInput)(NeuronExecution * execution, int32_t index,
                                            const NeuronOperandType* type, const void* buffer,
                                            size_t length);
    typedef int (*NeuronExecution_setOutput)(NeuronExecution * execution, int32_t index,
                                             const NeuronOperandType* type, void* buffer,
                                             size_t length);
    typedef int (*NeuronExecution_compute)(NeuronExecution * execution);

    // Open the shared library.
    // In Android, /system/system_ext/lib64/libneuronusdk_adapter.mtk.so is provided for apps.
    // Dlopen libneuronusdk_adapter.mtk.so if you are developing apps or components in /system.
    // Otherwise, dlopen libneuron_adapter.so when developing in Linux or /vendor in Android.
    //    handle = dlopen("/system/system_ext/lib64/libneuronusdk_adapter.mtk.so", RTLD_LAZY);
    handle = dlopen("libneuronusdk_adapter.mtk.so", RTLD_LAZY);
    if (handle == nullptr) {
        std::cerr << dlerror() << std::endl;
        std::cerr << "Failed to open libneuronusdk_adapter.mtk.so" << std::endl;
        exit(2);
    }

#define LOAD_FUNCTIONS(FUNC_NAME, VARIABLE_NAME)                        \
    FUNC_NAME VARIABLE_NAME = reinterpret_cast<FUNC_NAME>(load_func(handle, #FUNC_NAME))
    LOAD_FUNCTIONS(Neuron_getVersion, neuron_getVersion);
    LOAD_FUNCTIONS(NeuronModel_create, neuron_model_create);
    LOAD_FUNCTIONS(NeuronModel_free, neuron_model_free);
    LOAD_FUNCTIONS(NeuronModel_finish, neuron_model_finish);
    LOAD_FUNCTIONS(NeuronModel_addOperand, neuron_model_addOperand);
    LOAD_FUNCTIONS(NeuronModel_setOperandValue, neuron_model_setOperandValue);
    LOAD_FUNCTIONS(NeuronModel_addOperation, neuron_model_addOperation);
    LOAD_FUNCTIONS(NeuronModel_identifyInputsAndOutputs, neuron_model_identifyInputsAndOutputs);
    LOAD_FUNCTIONS(NeuronCompilation_create, neuron_compilation_create);
    LOAD_FUNCTIONS(NeuronCompilation_free, neuron_compilation_free);
    LOAD_FUNCTIONS(NeuronCompilation_finish, neuron_compilation_finish);
    LOAD_FUNCTIONS(NeuronExecution_create, neuron_execution_create);
    LOAD_FUNCTIONS(NeuronExecution_free, neuron_execution_free);
    LOAD_FUNCTIONS(NeuronExecution_setInput, neuron_execution_setInput);
    LOAD_FUNCTIONS(NeuronExecution_setOutput, neuron_execution_setOutput);
    LOAD_FUNCTIONS(NeuronExecution_compute, neuron_execution_compute);
#undef LOAD_FUNCTIONS

    NeuronRuntimeVersion version;
    (*neuron_getVersion)(&version);
    std::cout << "Neuron version " << static_cast<uint32_t>(version.major) << "."
              << static_cast<uint32_t>(version.minor) << "." << static_cast<uint32_t>(version.patch)
              << std::endl;

    NeuronModel* model = NULL;
    int neuron_errCode = (*neuron_model_create)(&model);
    if (NEURON_NO_ERROR != neuron_errCode) {
        std::cerr << "Fail to create model" << std::endl;
        exit(1);
    }

    NeuronOperandType tensor3x4Type;
    tensor3x4Type.type = NEURON_TENSOR_QUANT8_ASYMM;
    tensor3x4Type.scale = 0.5f; // For quantized tensors
    tensor3x4Type.zeroPoint = 0; // For quantized tensors
    tensor3x4Type.dimensionCount = 2;
    uint32_t dims[2] = {3, 4};
    tensor3x4Type.dimensions = dims;

    // We also specify operands that are activation function specifiers
    NeuronOperandType activationType;
    activationType.type = NEURON_INT32;
    activationType.scale = 0.f;
    activationType.zeroPoint = 0;
    activationType.dimensionCount = 0;
    activationType.dimensions = NULL;

    // Now we add the operands for required by current OP
    (*neuron_model_addOperand)(model, &tensor3x4Type); // operand 0
    (*neuron_model_addOperand)(model, &tensor3x4Type); // operand 1
    (*neuron_model_addOperand)(model, &activationType); // operand 2
    (*neuron_model_addOperand)(model, &tensor3x4Type); // operand 3

    const int sizeOfTensor = 3 * 4; // The formula for size calculation is dim0 * dim1 * elementSize
    uint8_t mem3x4[sizeOfTensor] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                    0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    (*neuron_model_setOperandValue)(model, 1, mem3x4, sizeOfTensor);
    // values of the activation operands
    int32_t noneValue = NEURON_FUSED_NONE;
    (*neuron_model_setOperandValue)(model, 2, &noneValue, sizeof(noneValue));

    // We have two operations in our example
    // The first consumes operands 1, 0, 2, and produces operand 3
    uint32_t addInputIndexes[3] = {1, 0, 2};
    uint32_t addOutputIndexes[1] = {3};
    (*neuron_model_addOperation)(model, NEURON_ADD, 3, addInputIndexes, 1, addOutputIndexes);

    (*neuron_model_addOperand)(model, &tensor3x4Type); // operand 4
    (*neuron_model_addOperand)(model, &activationType); // operand 5
    (*neuron_model_addOperand)(model, &tensor3x4Type); // operand 6
    (*neuron_model_setOperandValue)(model, 4, mem3x4, sizeOfTensor);
    (*neuron_model_setOperandValue)(model, 5, &noneValue, sizeof(noneValue));

    // The second consumes operands 3, 4, 5, and produces operand 6
    uint32_t multInputIndexes[3] = {3, 4, 5};
    uint32_t multOutputIndexes[1] = {6};
    (*neuron_model_addOperation)(model, NEURON_MUL, 3, multInputIndexes, 1, multOutputIndexes);

    // Our model has one input (0) and one output (6)
    uint32_t modelInputIndexes[1] = {0};
    uint32_t modelOutputIndexes[1] = {6};
    (*neuron_model_identifyInputsAndOutputs)(model, 1, modelInputIndexes, 1, modelOutputIndexes);
    (*neuron_model_finish)(model);

    // Compile the model
    NeuronCompilation* compilation;
    int ret = (*neuron_compilation_create)(model, &compilation);
    if (ret != NEURON_NO_ERROR) {
        std::cerr << "Failed to create compilation" << std::endl;
        exit(1);
    }
    if ((*neuron_compilation_finish)(compilation)) {
        std::cout << "Compilation failed" << std::endl;
        exit(1);
    }

    // Run the compiled model against a set of inputs
    NeuronExecution* run1 = NULL;
    (*neuron_execution_create)(compilation, &run1);
    // Set the single input to our sample model. Since it is small, we won't use a
    // memory buffer
    uint8_t myInput[3][4] = {
        {0x2, 0x2, 0x2, 0x2}, {0x2, 0x2, 0x2, 0x2}, {0x2, 0x2, 0x2, 0x2}};
    (*neuron_execution_setInput)(run1, 0, NULL, myInput, sizeof(myInput));
    // Set the output
    uint8_t myOutput[3][4];
    (*neuron_execution_setOutput)(run1, 0, NULL, myOutput, sizeof(myOutput));
    (*neuron_execution_compute)(run1);
    (*neuron_execution_free)(run1);
    (*neuron_compilation_free)(compilation);
    (*neuron_model_free)(model);

    // dump input and output
    std::cout << "Input: " << std::endl;
    std::string inputString;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            inputString += std::to_string((uint32_t)myInput[i][j]) + " ";
        }
        inputString += "\n";
    }
    std::cout << inputString << std::endl;
    std::cout << "Output: " << std::endl;
    std::string outputString;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            outputString += std::to_string((uint32_t)myOutput[i][j]) + " ";
        }
        outputString += "\n";
    }
    std::cout << outputString << std::endl;
    return 0;
}
