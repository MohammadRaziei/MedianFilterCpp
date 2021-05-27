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

inline void movingAverageFilterKernel(float output[], const float input[], const UINT len, const UINT halfWindow)
{
    const UINT windowSize = 2 * halfWindow + 1;
    float sum = 0;
    for (UINT i = 0; i < windowSize; ++i) sum += input[i];

    output[0] = sum / windowSize;
    for (UINT i = 1; i < len; ++i)
    {
        sum += (input[i + windowSize] - input[i - 1]);
        output[i] = sum / windowSize;
    }
}

inline void sortedInOut(float sortedData[], const UINT len, const float& outValue, const float& inValue)
{
    bool notFound = true;
    char allTasks = 2;
    float value = sortedData[0], saveValue;
    sortedData[len] = inValue;
    for (UINT j = 0, i = 0; i < len; ++i)
    {
        if (value == outValue)
        {
            value = sortedData[++j];
            --allTasks;
        }
        if (notFound && value >= inValue)
        {
            sortedData[i] = inValue;
            notFound = false;
            --allTasks;
        }
        else
        {
            saveValue = value;
            value = sortedData[++j];
            sortedData[i] = saveValue;
        }
        if (allTasks <= 0) break;
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

void movingFilter(std::vector<float>& output,
     const std::vector<float>& input,
     const UINT halfWindow,
     const std::function<void(float[], const float[], const UINT, const UINT)>& filtKernel,
     const ParallelMethod& method = ParallelMethod::NONE)
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
            filtKernel(output.data(), inp.data(), (UINT)vecSize, halfWindow);
            break;
        }
        case ParallelMethod::CPU:
        {
            const UINT lenFrames = (UINT)(vecSize / 100);
            Concurrency::parallel_for((UINT)0, (UINT)vecSize, lenFrames, [&](const UINT& i) {
                filtKernel(output.data() + i, inp.data() + i, lenFrames, halfWindow);
            });
            break;
        }
        case ParallelMethod::GPU:
        {
            break;
        }
    }
    stop = chronoTime::now();
    std::cout << "\tElapsed Time: " << std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1e3 << " ms" << std::endl;
}

inline float myrand()
{
    return (float)std::rand() / (RAND_MAX);
}

int main()
{
    std::vector<float> data(500000);
    std::generate(data.begin(), data.end(), []() { return myrand() < 0.001 ? 100 * myrand() : myrand(); });
    std::vector<float> filtData(data.size());
    const UINT halfWindowSize = 75;

    io::writeToFile("input.bin", data);

    std::cout << "* Moving-Average Filter:" << std::endl;
    std::cout << ">> ParallelMethod::NONE" << std::endl;
    movingFilter(filtData, data, halfWindowSize, movingAverageFilterKernel, ParallelMethod::NONE);
    io::writeToFile("output-meanNONE.bin", filtData);
    std::cout << ">> ParallelMethod::CPU" << std::endl;
    movingFilter(filtData, data, halfWindowSize, movingAverageFilterKernel, ParallelMethod::CPU);
    io::writeToFile("output-meanCPU.bin", filtData);

    std::cout << std::endl;

    std::cout << "* Median Filter:" << std::endl;
    std::cout << ">> ParallelMethod::NONE" << std::endl;
    movingFilter(filtData, data, halfWindowSize, medianFilterKernel, ParallelMethod::NONE);
    io::writeToFile("output-medianNONE.bin", filtData);
    std::cout << ">> ParallelMethod::CPU" << std::endl;
    movingFilter(filtData, data, halfWindowSize, medianFilterKernel, ParallelMethod::CPU);
    io::writeToFile("output-medianCPU.bin", filtData);

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
