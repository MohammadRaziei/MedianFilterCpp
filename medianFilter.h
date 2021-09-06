#ifndef MEDIANFILTER_H
#define MEDIANFILTER_H

#include <stdint.h>

enum class ParallelMethod { NONE, CPU, GPU };

namespace utils {
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

template <uint32_t NUM_THREADS = 8, typename T>
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
  std::cout << "\tElapsed Time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(stop -
                                                                     start)
                       .count() /
                   1000.f
            << " ms" << std::endl;
}
}  // namespace utils

#endif  // MEDIANFILTER_H
