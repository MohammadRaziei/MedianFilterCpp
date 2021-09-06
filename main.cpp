#include <ppl.h>
#include <stdint.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "io.h"
#include "medianFilter.h"

#define getMiliSeconds(t) \
  (std::chrono::duration_cast<std::chrono::microseconds>(t).count() / 1000.f)

#define NUM_THREADS (8)
typedef std::chrono::high_resolution_clock chronoTime;
inline float myrand() { return (float)std::rand() / (RAND_MAX); }

int test() {
  float a[6]{0, 0, 0, 0, 3};
  int N = 5;
  printArray(a, N);
  float aout = 0;
  float ain = 5;
  show(aout);
  show(ain);
  utils::sortedInOut(a, N, aout, ain);
  printArray(a, N);
  return 0;
}

int main() {
  //    return test();
  std::vector<float> data(2'096'127);
  std::generate(data.begin(), data.end(),
                []() { return myrand() < 0.0001 ? 100 * myrand() : myrand(); });
  std::vector<float> filtData(data.size());
  const uint32_t halfWindowSize = 75;
  io::writeTextFile("halfWindowSize.txt", halfWindowSize);

  io::writeToFile("input.bin", data);

  std::cout << "* Moving-Average Filter:" << std::endl;
  std::cout << ">> ParallelMethod::NONE" << std::endl;
  utils::movingFilter<NUM_THREADS>(filtData, data, halfWindowSize,
                                   utils::movingAverageFilterKernel,
                                   ParallelMethod::NONE);
  io::writeToFile("output-meanNONE.bin", filtData);
  std::cout << ">> ParallelMethod::CPU" << std::endl;
  utils::movingFilter<NUM_THREADS>(filtData, data, halfWindowSize,
                                   utils::movingAverageFilterKernel,
                                   ParallelMethod::CPU);
  io::writeToFile("output-meanCPU.bin", filtData);

  std::cout << std::endl;

  std::cout << "* Median Filter:" << std::endl;
  std::cout << ">> ParallelMethod::NONE" << std::endl;
  utils::movingFilter<NUM_THREADS>(filtData, data, halfWindowSize,
                                   utils::medianFilterKernel,
                                   ParallelMethod::NONE);
  io::writeToFile("output-medianNONE.bin", filtData);
  std::cout << ">> ParallelMethod::CPU" << std::endl;
  utils::movingFilter<NUM_THREADS>(filtData, data, halfWindowSize,
                                   utils::medianFilterKernel,
                                   ParallelMethod::CPU);
  io::writeToFile("output-medianCPU.bin", filtData);
}
