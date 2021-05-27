#pragma once
#include <stdint.h>
#include <algorithm>
#include <complex>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#define compT std::complex<T>
#define compFloat std::complex<float>
#define compDouble std::complex<double>
#define vectT std::vector<T>
#define vectFloat std::vector<float>
#define vectDouble std::vector<double>
#define vectCompT std::vector<compT>
#define vectCompFloat std::vector<compFloat>
#define vectCompDouble std::vector<compDouble>
#define vectStr std::vector<std::string>

#define PI 3.1415926535

// PRINT
#define eprintf(format, ...) fprintf(stderr, format __VA_OPT__(, ) __VA_ARGS__)
#define show(x) std::cout << #x << ": " << x << std::endl
#ifndef Max_Print_Vector_Size
#define Max_Print_Vector_Size (10)
#endif // Max_Print_Vector_Size

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    os << "[";
    int counter = 0;
    for (typename std::vector<T>::const_iterator it = v.begin();
         it != v.end() - 1;
         ++it)
    {
        if (counter++ == 10) break;
        os << *it << ", ";
    }
    os << *v.rbegin() << "]";
    return os;
}

void print() noexcept
{
    std::cout << std::endl;
}
template <typename T>
void print(T a) noexcept
{
    std::cout << a;
}
template <typename T, class... ARG>
void print(T a, ARG... arg) noexcept
{
    print(a);
    std::cout << ", ";
    print(arg...);
    std::cout << std::endl;
}
template <typename T>
void print(const vectT& a) noexcept
{
    std::cout << vectT(a.begin(),
                      a.begin() +
                           std::min<size_t>(a.size(), Max_Print_Vector_Size))
              << std::endl;
}

namespace io
{
template <typename T>
void readFromFile(const std::string filename,
     std::vector<T>& vec,
     size_t length = (size_t)(-1),
     size_t* start = nullptr) noexcept
{
    // open the file:
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open())
    {
        fprintf(stderr, "Cannot open: \"%s\"\n", filename.c_str());
        infile.close();
        return;
    }
    // Stop eating new lines in binary mode!!!
    infile.unsetf(std::ios::skipws);

    const size_t startByte = start ? *start * sizeof(T) : 0l;
    // get its size:
    infile.seekg(0, std::ios::end);
    size_t fileSizeByte = infile.tellg();
    if (startByte > fileSizeByte)
    {
        fprintf(stderr, "startByte > fileSizeByte\n");
        infile.close();
        return;
    }
    infile.seekg(startByte, std::ios::beg);
    fileSizeByte -= startByte;

    size_t fileSize = fileSizeByte / sizeof(T);
    fileSize = std::min<size_t>(fileSize, length);
    if (start) *start += fileSize;

    vec.resize(fileSize);
    infile.read(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
    infile.close();
}
template <typename T = float>
std::vector<T> readFromFile(const std::string filename,
     size_t length = (size_t)(-1),
     size_t* start = nullptr) noexcept
{
    std::vector<T> buf;
    readFromFile(filename, buf, length, start);
    return buf;
}

template <typename T>
void readStereoFromFile(const std::string& filename, std::vector<T>& vecReal, std::vector<T>& vecImag, const size_t length = (size_t)-1, size_t* start = nullptr)
{
    // open the file:
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open())
    {
        fprintf(stderr, "Cannot open: \"%s\"\n", filename.c_str());
        infile.close();
        return;
    }
    // Stop eating new lines in binary mode!!!
    infile.unsetf(std::ios::skipws);

    const size_t startByte = start ? *start * sizeof(T) : 0l;
    // get its size:
    infile.seekg(0, std::ios::end);
    size_t fileSizeByte = infile.tellg();
    if (startByte > fileSizeByte)
    {
        fprintf(stderr, "startByte > fileSizeByte\n");
        infile.close();
        return;
    }
    infile.seekg(startByte, std::ios::beg);
    fileSizeByte -= startByte;

    size_t fileSize = fileSizeByte / sizeof(T);
    fileSize = std::min<size_t>(fileSize, length * 2);
    if (start) *start += fileSize;
    fileSize /= 2;

    vecReal.resize(fileSize);
    vecImag.resize(fileSize);

    T* vecRealPtr{vecReal.data()};
    T* vecImagPtr{vecImag.data()};
    T readingVar;
    char* readingChar = reinterpret_cast<char*>(&readingVar);
    size_t i = 0;
    while ((!infile.eof()) && (++i < fileSize))
    {
        infile.read(readingChar, sizeof(T));
        *(vecRealPtr++) = readingVar;
        infile.read(readingChar, sizeof(T));
        *(vecImagPtr++) = readingVar;
    }
    vecReal.resize(i);
    vecImag.resize(i);
    infile.close();
}

std::string readTextFile(const std::string& filename) noexcept
{
    std::ifstream infile(filename);
    std::string src((std::istreambuf_iterator<char>(infile)),
         (std::istreambuf_iterator<char>()));
    infile.close();
    return src;
}

template <typename T>
void writeToFile(const std::string& filename, const vectT& data)
{
    std::ofstream outFile(filename, std::ios::out | std::ofstream::binary);
    if (!outFile.is_open())
    {
        fprintf(stderr, "Cannot open: \"%s\"\n", filename.c_str());
        return;
    }
    outFile.write(reinterpret_cast<const char*>(data.data()),
         sizeof(T) * data.size());
    outFile.close();
}
} // namespace io

/// algorithms
namespace alg
{
template <typename InType, typename OutType>
inline void splitStereoToMono(const std::vector<std::complex<InType>> signal,
     std::vector<OutType>& vecReal,
     std::vector<OutType>& vecImag)
{
    const size_t vecSize{signal.size()};
    vecReal.resize(vecSize);
    vecImag.resize(vecSize);
    for (size_t i = 0; i < vecSize; ++i)
    {
        vecReal[i] = (OutType)signal[i].real();
        vecImag[i] = (OutType)signal[i].imag();
    }
}

template <typename T>
vectCompT mixMonoToStereo(const vectT& vecReal, const vectT& vecImag)
{
    const size_t vecSize{vecReal.size()};
    vectCompT vecComp(vecSize);
    for (size_t i = 0; i < vecSize; ++i)
        vecComp[i] = compT(vecReal[i], vecImag[i]);
    return vecComp;
}

template <typename T>
T median(vectT v)
{
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    return v[v.size() / 2];
}

template <typename T>
T median(T* v_begin, T* v_end)
{
    const int v_size2{static_cast<int>(v_end - v_begin) / 2};
    std::nth_element(v_begin, v_begin + v_size2, v_end);
    return v_begin[v_size2];
}

} // namespace alg
