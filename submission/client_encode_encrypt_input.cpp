#include "BenchmarkUtils.hpp"
#include "WordEncoder.hpp"

#include <iostream>

static std::vector<std::vector<u64>>
readWordsText(const std::filesystem::path &path, u32 total_batches,
              u32 num_words) {
  std::ifstream is(path);
  if (!is)
    throw std::runtime_error("cannot open " + path.string());
  std::vector<std::vector<u64>> out(total_batches,
                                    std::vector<u64>(num_words, 0));
  u64 v = 0;
  size_t idx = 0;
  while (is >> v) {
    size_t b = idx / num_words;
    size_t w = idx % num_words;
    if (b >= total_batches)
      break;
    out[b][w] = v;
    ++idx;
  }
  return out;
}

int main(int argc, char **argv) {
  std::string instance;
  u32 batch_size;
  parseArgs(argc, argv, "client_encode_encrypt_input", instance, batch_size);
  u64 num_target_words = dataSizeFromInstance(instance);

  const auto params = getParamsSet();
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);
  u32 total_batches = num_cts * batch_size;

  auto io = ioDir(instance);
  auto ddir = datasetDir(instance);

  auto t_read = Clock::now();
  auto all_words0 = readWordsText(ddir / "lhs.txt", total_batches, num_words);
  auto all_words1 = readWordsText(ddir / "rhs.txt", total_batches, num_words);
  log_time("read input texts", elapsed_ms(t_read));

  auto t_setup = Clock::now();
  WordEncodeParams word_ecd_params;
  word_ecd_params.setLogDegree(log_degree);
  word_ecd_params.setBatchSize(batch_size);
  WordEncoder word_encoder(word_ecd_params);

  EnDecoder encoder(params.ecd_params);

  std::ifstream sk_is((io / "secret_keys" / "sk.bin").string(),
                      std::ios::binary);
  if (!sk_is)
    throw std::runtime_error("cannot read secret key");
  auto sk = serial::loadAsPtr<ISecretKey>(sk_is);

  EnDecryptor encryptor(params.enc_params);
  ensureDir(io / "ciphertexts_upload");
  log_time("setup (encoder/sk/encryptor)", elapsed_ms(t_setup));

  double total_encode_ms = 0, total_encrypt_ms = 0, total_write_ms = 0;

  for (u32 c = 0; c < num_cts; ++c) {
    u32 offset = c * batch_size;
    std::vector<std::vector<u64>> words0(
        all_words0.begin() + offset, all_words0.begin() + offset + batch_size);
    std::vector<std::vector<u64>> words1(
        all_words1.begin() + offset, all_words1.begin() + offset + batch_size);

    auto t_enc = Clock::now();
    std::vector<Message> msgs0, msgs1;
    word_encoder.wordToComplex(words0, msgs0);
    word_encoder.wordToComplex(words1, msgs1);

    auto ptxt0 = IPlaintext::make(getPtxtType(batch_size));
    auto ptxt1 = IPlaintext::make(getPtxtType(batch_size));
    encodeBatch(encoder, msgs0, *ptxt0, 2);
    encodeBatch(encoder, msgs1, *ptxt1, 2);
    total_encode_ms += elapsed_ms(t_enc);

    auto t_encrypt = Clock::now();
    auto ctxt0 = ICiphertext::make(getEncType(batch_size));
    auto ctxt1 = ICiphertext::make(getEncType(batch_size));
    encryptor.encrypt(*ptxt0, *sk, *ctxt0);
    encryptor.encrypt(*ptxt1, *sk, *ctxt1);
    total_encrypt_ms += elapsed_ms(t_encrypt);

    auto t_write = Clock::now();
    serial::save(
        (io / "ciphertexts_upload" / ("ctxt0_" + std::to_string(c) + ".bin"))
            .string(),
        *ctxt0);
    serial::save(
        (io / "ciphertexts_upload" / ("ctxt1_" + std::to_string(c) + ".bin"))
            .string(),
        *ctxt1);
    total_write_ms += elapsed_ms(t_write);
  }

  log_time("encode (wordToComplex+encode)", total_encode_ms);
  log_time("encrypt", total_encrypt_ms);
  log_time("write ciphertexts", total_write_ms);
  return 0;
}
