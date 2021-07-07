#include <ppl.h>
#include <stdint.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "io.h"

#define getMiliSeconds(t) \
  (std::chrono::duration_cast<std::chrono::microseconds>(t).count() / 1000.f)

#define NUM_THREADS (128)
typedef std::chrono::high_resolution_clock chronoTime;

enum class ParallelMethod { NONE, CPU, GPU };

template <typename T>
inline void movingAverageFilterKernel(T output[], const T input[],
                                      const uint32_t len,
                                      const uint32_t halfWindow) {
  const uint32_t windowSize = 2 * halfWindow + 1;
  T sum = 0;
  for (uint32_t i = 0; i < windowSize; ++i) sum += input[i];
  output[0] = sum / windowSize;
  for (uint32_t i = 0; i < len - 1; ++i) {
    sum += (input[i + windowSize] - input[i]);
    output[i + 1] = sum / windowSize;
  }
}

template <typename T>
inline void sortedInOut(T sortedData[], const uint32_t len, const T& outValue,
                        const T& inValue) {
  bool TasksOut = true, TasksIn = true;
  if (outValue == inValue) return;
  //    std::vector<T> sortedData(len + 1);
  //    std::copy_n(sortedData2, len + 1, sortedData.data());
  sortedData[len] = inValue;
  T value = sortedData[0], saveValue;
  for (uint32_t j = 0, i = 0; (TasksOut || TasksIn) && i < len; ++i) {
    if (TasksOut && value == outValue) {
      value = sortedData[++j];
      TasksOut = false;
    }
    if (TasksIn && value >= inValue) {
      sortedData[i] = inValue;
      TasksIn = false;
    } else {
      saveValue = value;
      value = sortedData[++j];

      sortedData[i] = saveValue;
    }
  }
}

template <typename T>
inline void medianFilterKernel(T output[], const T input[], const uint32_t len,
                               const uint32_t halfWindow) {
  const uint32_t windowSize = 2 * halfWindow + 1;
  T* temp = new T[windowSize + 1];
  memcpy(temp, input, windowSize * sizeof(T));

  std::sort(temp, temp + windowSize);
  output[0] = temp[halfWindow];
  for (uint32_t i = 0; i < len - 1; ++i) {
    sortedInOut(temp, windowSize, input[i], input[i + windowSize]);
    output[i + 1] = temp[halfWindow];
  }
  delete[] temp;
}

template <typename T>
void movingFilter(std::vector<T>& output, const std::vector<T>& input,
                  const uint32_t halfWindow,
                  void (&filtKernel)(T[], const T[], const uint32_t,
                                     const uint32_t),
                  const ParallelMethod& method = ParallelMethod::NONE) {
  const size_t vecSize = input.size();
  const uint32_t windowSize = 2 * halfWindow + 1;
  std::vector<T> inp(vecSize + windowSize);
  memset(inp.data(), 0, (windowSize - 1) * sizeof(T));
  memcpy(inp.data() + windowSize - 1, input.data(), vecSize * sizeof(T));
  output.resize(vecSize);
  chronoTime::time_point start, stop;
  start = chronoTime::now();
  switch (method) {
    case ParallelMethod::NONE: {
      filtKernel(output.data(), inp.data(), (uint32_t)vecSize, halfWindow);
      break;
    }
    case ParallelMethod::CPU: {
      const uint32_t lenFrames = (uint32_t)ceilf((T)vecSize / NUM_THREADS);
      Concurrency::parallel_for(
          (uint32_t)0, (uint32_t)vecSize, lenFrames, [&](const uint32_t& i) {
            if (i + lenFrames <= vecSize)
              filtKernel(output.data() + i, inp.data() + i, lenFrames,
                         halfWindow);
            else
              filtKernel(output.data() + i, inp.data() + i,
                         (uint32_t)vecSize - i, halfWindow);
          });
      break;
    }
    case ParallelMethod::GPU: {
      break;
    }
  }
  output.resize(vecSize);
  stop = chronoTime::now();

  std::cout << "\tElapsed Time: " << getMiliSeconds(stop - start) << " ms"
            << std::endl;
}

inline float myrand() { return (float)std::rand() / (RAND_MAX); }

int test() {
  float a[6]{0, 0, 0, 0, 3};
  int N = 5;
  printArray(a, N);
  float aout = 0;
  float ain = 5;
  show(aout);
  show(ain);
  sortedInOut(a, N, aout, ain);
  printArray(a, N);
  return 0;
}

int main() {
  //    return test();
  std::vector<float> data(500'000);
  std::generate(data.begin(), data.end(),
                []() { return myrand() < 0.0001 ? 100 * myrand() : myrand(); });
  std::vector<float> filtData(data.size());
  const uint32_t halfWindowSize = 75;
  io::writeTextFile("halfWindowSize.txt", halfWindowSize);

  io::writeToFile("input.bin", data);

  std::cout << "* Moving-Average Filter:" << std::endl;
  std::cout << ">> ParallelMethod::NONE" << std::endl;
  movingFilter(filtData, data, halfWindowSize, movingAverageFilterKernel,
               ParallelMethod::NONE);
  io::writeToFile("output-meanNONE.bin", filtData);
  std::cout << ">> ParallelMethod::CPU" << std::endl;
  movingFilter(filtData, data, halfWindowSize, movingAverageFilterKernel,
               ParallelMethod::CPU);
  io::writeToFile("output-meanCPU.bin", filtData);

  std::cout << std::endl;

  std::cout << "* Median Filter:" << std::endl;
  std::cout << ">> ParallelMethod::NONE" << std::endl;
  movingFilter(filtData, data, halfWindowSize, medianFilterKernel,
               ParallelMethod::NONE);
  io::writeToFile("output-medianNONE.bin", filtData);
  std::cout << ">> ParallelMethod::CPU" << std::endl;
  movingFilter(filtData, data, halfWindowSize, medianFilterKernel,
               ParallelMethod::CPU);
  io::writeToFile("output-medianCPU.bin", filtData);
}
