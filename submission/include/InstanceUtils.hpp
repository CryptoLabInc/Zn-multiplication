#pragma once

#include "HEaaN2/HEaaN2.hpp"

#include <filesystem>

using namespace heaan;

inline u64 dataSizeFromInstance(const std::string &s) {
  if (s == "single")
    return 1;
  if (s == "small")
    return 1'000;
  if (s == "medium")
    return 100'000;
  if (s == "large")
    return 10'000'000;
  throw std::runtime_error("unknown instance: " + s);
}

inline std::string instanceFromDataSize(const std::string &s) {
  if (s == "1")
    return "single";
  if (s == "1000")
    return "small";
  if (s == "100000")
    return "medium";
  if (s == "10000000")
    return "large";
  throw std::runtime_error("unknown data size: " + s);
}

inline u32 defaultBatchSizeFromInstance(const std::string &s) {
  if (s == "single" || s == "small")
    return 1;
  if (s == "medium" || s == "large")
    return 64;
  throw std::runtime_error("unknown instance: " + s);
}

inline std::filesystem::path datasetDir(const std::string &instance) {
  return std::filesystem::path("datasets") / instance;
}

inline std::filesystem::path ioDir(const std::string &instance) {
  return std::filesystem::path("io") / instance;
}

inline void ensureDir(const std::filesystem::path &p) {
  std::filesystem::create_directories(p);
}

inline void parseArgs(int argc, char **argv, const char *prog,
                      std::string &instance, u64 &num_target_words,
                      u32 &batch_size) {
  if (argc < 2) {
    std::cerr << "Usage: " << prog << " <instance> [data_size] [batch_size]"
              << std::endl;
    exit(1);
  }
  instance = argv[1];
  num_target_words = dataSizeFromInstance(instance);
  if (argc >= 3) {
    u64 data_size = std::stoull(argv[2]);
    if (data_size != num_target_words)
      throw std::runtime_error("data_size (" + std::to_string(data_size) +
                               ") does not match instance " + instance + " (" +
                               std::to_string(num_target_words) + ")");
  }
  batch_size = (argc >= 4) ? static_cast<u32>(std::stoul(argv[3]))
                           : defaultBatchSizeFromInstance(instance);
}

inline void parseArgs(int argc, char **argv, const char *prog,
                      std::string &instance, u32 &batch_size) {
  if (argc < 2) {
    std::cerr << "Usage: " << prog << " <instance> [batch_size]" << std::endl;
    exit(1);
  }
  instance = argv[1];
  batch_size = (argc >= 3) ? static_cast<u32>(std::stoul(argv[2]))
                           : defaultBatchSizeFromInstance(instance);
}
