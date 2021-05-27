// Example program
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ppl.h>

#include "io.h"

#define UINT unsigned int
#define MaxMedianWindow (300)
typedef std::chrono::high_resolution_clock chronoTime;
#define chronoMicrosecond(x) std::chrono::duration_cast<std::chrono::microseconds>(x).count()

enum class ParallelMethod
{
    NONE,
    CPU,
    GPU
};

inline void sortedInOut(float sortedData[], const UINT len, const float& outValue, const float& inValue)
{
    bool notFound = true;
    float value = sortedData[0], saveValue;
    sortedData[len] = inValue;
    for (UINT j = 0, i = 0; i < len; ++i)
    {
        if (value == outValue)
            value = sortedData[++j];
        if (notFound && value >= inValue)
        {
            sortedData[i] = inValue;
            notFound = false;
        }
        else
        {
            saveValue = value;
            value = sortedData[++j];
            sortedData[i] = saveValue;
        }
    }
}
inline void medianFilterKernel(float output[], const float input[], const UINT len, const UINT halfWindow)
{
    const UINT halfWindow1 = halfWindow + 1;
    const UINT windowSize = 2 * halfWindow + 1;
    float* temp = new float[windowSize + 1];
    //    float temp[MaxMedianWindow];
    memcpy(temp, input, windowSize * sizeof(float));
    std::sort(temp, temp + windowSize);
    output[0] = temp[halfWindow1];
    for (UINT i = 1; i < len; ++i)
    {
        sortedInOut(temp, windowSize, input[i - 1], input[i + windowSize]);
        output[i] = temp[halfWindow1];
    }
    delete[] temp;
}
void medianFilter(std::vector<float>& output, const std::vector<float>& input, const UINT halfWindow, const ParallelMethod& method = ParallelMethod::NONE)
{
    const size_t vecSize = input.size();
    const UINT windowSize = 2 * halfWindow + 1;
    std::vector<float> inp(vecSize + windowSize);
    memcpy(inp.data() + halfWindow + 1, input.data(), vecSize * sizeof(float));
    output.resize(vecSize);
    chronoTime::time_point start, stop;
    start = chronoTime::now();
    switch (method)
    {
        case ParallelMethod::NONE:
        {
            medianFilterKernel(output.data(), inp.data(), (UINT)vecSize, halfWindow);
            break;
        }
        case ParallelMethod::CPU:
        {
            const UINT lenFrames = (UINT)(vecSize / 100);
            Concurrency::parallel_for((UINT)0, (UINT)vecSize, lenFrames, [&](const UINT& i) {
                medianFilterKernel(output.data() + i, inp.data() + i, lenFrames, halfWindow);
            });
            break;
        }
        case ParallelMethod::GPU:
        {
            break;
        }
    }
    stop = chronoTime::now();
    std::cout << "Elapsed Time: " << std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1e3 << " ms" << std::endl;
}

inline float myrand()
{
    return (float)std::rand() / (RAND_MAX);
}

int main()
{
    std::vector<float> data(500000);
    std::generate(data.begin(), data.end(), []() { return myrand() < 0.01 ? 1 + myrand() : myrand(); });
    std::vector<float> filtData(data.size());

    io::writeToFile("input.bin", data);

    std::cout << ">> ParallelMethod::NONE" << std::endl;
    medianFilter(filtData, data, 75, ParallelMethod::NONE);
    io::writeToFile("output-NONE.bin", filtData);
    std::cout << ">> ParallelMethod::CPU" << std::endl;
    medianFilter(filtData, data, 75, ParallelMethod::CPU);
    io::writeToFile("output-CPU.bin", filtData);

    //    float sortedData[6] = {1, 2, 3, 4, 5};
    //    const UINT len = sizeof(sortedData) / sizeof(float) - 1;
    //    for (UINT i = 0; i < len; i++)
    //    {
    //        printf("%0.1f, ", sortedData[i]);
    //    }
    //    std::cout << std::endl;
    //    sortedInOut(sortedData, len, 2.5, 5);
    //    std::cout << std::endl;

    //    for (UINT i = 0; i < len; i++)
    //    {
    //        printf("%0.1f, ", sortedData[i]);
    //    }
    //    std::cout << std::endl;
}
