#include "BenchmarkUtils.hpp"
#include "ParallelSchedule.hpp"
#include "WordEncoder.hpp"

#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <omp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static std::vector<std::vector<u64>>
readWordsText(const std::filesystem::path &path, u32 total_batches,
              u32 num_words) {
  std::vector<std::vector<u64>> out(total_batches,
                                    std::vector<u64>(num_words, 0));
  const u64 total = static_cast<u64>(total_batches) * num_words;

  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0)
    throw std::runtime_error("cannot open " + path.string());
  struct stat st {};
  if (fstat(fd, &st) != 0) {
    close(fd);
    throw std::runtime_error("cannot stat " + path.string());
  }
  size_t sz = static_cast<size_t>(st.st_size);
  if (sz == 0) {
    close(fd);
    return out;
  }
  const char *data = static_cast<const char *>(
      mmap(nullptr, sz, PROT_READ, MAP_PRIVATE, fd, 0));
  close(fd);
  if (data == MAP_FAILED)
    throw std::runtime_error("mmap failed for " + path.string());
  madvise(const_cast<char *>(data), sz, MADV_SEQUENTIAL);

  auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
  const int nthreads = std::max(1, omp_get_max_threads());
  auto chunk = [&](int t) { return static_cast<size_t>(sz) * t / nthreads; };

  std::vector<u64> counts(nthreads, 0);
#pragma omp parallel num_threads(nthreads)
  {
    int t = omp_get_thread_num();
    size_t lo = chunk(t), hi = chunk(t + 1);
    u64 cnt = 0;
    for (size_t i = lo; i < hi; ++i)
      if (is_digit(data[i]) && (i == 0 || !is_digit(data[i - 1])))
        ++cnt;
    counts[t] = cnt;
  }
  std::vector<u64> starts(nthreads, 0);
  for (int t = 1; t < nthreads; ++t)
    starts[t] = starts[t - 1] + counts[t - 1];

#pragma omp parallel num_threads(nthreads)
  {
    int t = omp_get_thread_num();
    size_t lo = chunk(t), hi = chunk(t + 1);
    u64 idx = starts[t];
    size_t row = idx / num_words, col = idx % num_words;
    for (size_t i = lo; i < hi;) {
      if (!is_digit(data[i]) || (i != 0 && is_digit(data[i - 1]))) {
        ++i;
        continue;
      }
      u64 v = 0;
      for (; i < sz && is_digit(data[i]); ++i)
        v = v * 10u + static_cast<u64>(data[i] - '0');
      if (idx < total) {
        out[row][col] = v;
        if (++col == num_words) {
          col = 0;
          ++row;
        }
        ++idx;
      }
    }
  }
  munmap(const_cast<char *>(data), sz);
  return out;
}

int main(int argc, char **argv) {
  std::string instance;
  u32 batch_size;
  parseArgs(argc, argv, "client_encode_encrypt_input", instance, batch_size);
  u64 num_target_words = dataSizeFromInstance(instance);

  const auto params = getParamsSet(instance);
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);
  u32 total_batches = num_cts * batch_size;

  auto io = ioDir(instance);
  auto ddir = datasetDir(instance);

  auto t_read = Clock::now();
  auto all_words0 = readWordsText(ddir / "lhs.txt", total_batches, num_words);
  auto all_words1 = readWordsText(ddir / "rhs.txt", total_batches, num_words);
  log_time("Read input texts", elapsed_ms(t_read));

  auto t_setup = Clock::now();
  WordEncodeParams word_ecd_params;
  word_ecd_params.setLogDegree(log_degree);
  word_ecd_params.setBatchSize(batch_size);

  std::ifstream sk_is((io / "secret_keys" / "sk.bin").string(),
                      std::ios::binary);
  if (!sk_is)
    throw std::runtime_error("cannot read secret key");
  auto sk = serial::loadAsPtr<ISecretKey>(sk_is);

  ensureDir(io / "ciphertexts_upload");
  log_time("Setup", elapsed_ms(t_setup));

  auto process_operand = [&](u32 c, int which, WordEncoder &word_encoder,
                             EnDecoder &encoder, EnDecryptor &encryptor) {
    u32 offset = c * batch_size;
    const auto &src = (which == 0) ? all_words0 : all_words1;
    std::vector<std::vector<u64>> words(src.begin() + offset,
                                        src.begin() + offset + batch_size);
    std::vector<Message> msgs;
    word_encoder.wordToComplex(words, msgs);
    auto ptxt = IPlaintext::make(getPtxtType(batch_size));
    encodeBatch(encoder, msgs, *ptxt, 2);
    auto ctxt = ICiphertext::make(getEncType(batch_size));
    encryptor.encrypt(*ptxt, *sk, *ctxt);
    serial::save(
        (io / "ciphertexts_upload" /
         ("ctxt" + std::to_string(which) + "_" + std::to_string(c) + ".bin"))
            .string(),
        *ctxt);
  };

  struct Engines {
    WordEncoder word_encoder;
    EnDecoder encoder;
    EnDecryptor encryptor;
  };
  const u32 ntasks = 2u * num_cts;
  const int outer = (num_cts == 1) ? 1 : outerThreads(ntasks);
  auto t_loop = Clock::now();
  runFheTasks(
      ntasks, outer,
      [&] {
        return Engines{WordEncoder(word_ecd_params),
                       EnDecoder(params.ecd_params),
                       EnDecryptor(params.enc_params)};
      },
      [&](u32 t, Engines &e) {
        process_operand(t % num_cts, t / num_cts, e.word_encoder, e.encoder,
                        e.encryptor);
      });
  log_time("Encode+Encrypt+Write", elapsed_ms(t_loop));
  return 0;
}
