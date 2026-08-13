#include "BenchmarkUtils.hpp"
#include "ParallelSchedule.hpp"
#include "WordEncoder.hpp"

#include <charconv>

int main(int argc, char **argv) {
  std::string instance;
  u64 num_target_words;
  u32 batch_size;
  parseArgs(argc, argv, "client_decrypt_decode", instance, num_target_words,
            batch_size);

  const auto params = getParamsSet(instance);
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);

  auto io = ioDir(instance);

  auto t_read_sk = Clock::now();
  auto sk =
      serial::loadAsPtr<ISecretKey>((io / "secret_keys" / "sk.bin").string());
  log_time("Read secret key", elapsed_ms(t_read_sk));

  auto t_setup = Clock::now();
  WordEncodeParams word_ecd_params;
  word_ecd_params.setLogDegree(log_degree);
  word_ecd_params.setBatchSize(batch_size);
  log_time("Setup", elapsed_ms(t_setup));

  auto out_dir = io / "cleartext_output";
  ensureDir(out_dir);

  const u64 words_per_ct = static_cast<u64>(batch_size) * num_words;

  std::vector<std::string> out_bufs(num_cts);
  auto process_ct = [&](u32 c, EnDecryptor &encryptor, EnDecoder &encoder,
                        WordEncoder &word_encoder) {
    const u64 start = static_cast<u64>(c) * words_per_ct;
    if (start >= num_target_words)
      return; // this ciphertext contributes no output
    const u64 cnt = std::min<u64>(words_per_ct, num_target_words - start);

    auto ctxt = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_download" / ("res_" + std::to_string(c) + ".bin"))
            .string());
    auto ptxt = IPlaintext::make(getPtxtType(batch_size));
    encryptor.decrypt(*ctxt, *sk, *ptxt);

    std::vector<Message> msgs;
    decodeBatch(encoder, *ptxt, msgs, batch_size);
    std::vector<std::vector<u64>> words;
    word_encoder.complexToWord(msgs, words);

    std::string &buf = out_bufs[c];
    buf.reserve(static_cast<size_t>(cnt) * 21);
    char tmp[24];
    u64 emitted = 0;
    for (u32 b = 0; b < batch_size && emitted < cnt; ++b) {
      for (u32 w = 0; w < num_words && emitted < cnt; ++w) {
        auto [ptr, ec] = std::to_chars(tmp, tmp + sizeof(tmp), words[b][w]);
        buf.append(tmp, static_cast<size_t>(ptr - tmp));
        buf.push_back('\n');
        ++emitted;
      }
    }
  };

  struct Engines {
    EnDecryptor encryptor;
    EnDecoder encoder;
    WordEncoder word_encoder;
  };
  auto t_loop = Clock::now();
  runFheTasks(
      num_cts, outerThreads(num_cts),
      [&] {
        return Engines{EnDecryptor(params.enc_params),
                       EnDecoder(params.ecd_params),
                       WordEncoder(word_ecd_params)};
      },
      [&](u32 c, Engines &e) {
        process_ct(c, e.encryptor, e.encoder, e.word_encoder);
      });
  log_time("Decrypt+Decode", elapsed_ms(t_loop));

  auto t_w = Clock::now();
  std::ofstream os(out_dir / "out.txt", std::ios::binary);
  if (!os)
    throw std::runtime_error("cannot write cleartext output");
  for (u32 c = 0; c < num_cts; ++c)
    if (!out_bufs[c].empty())
      os.write(out_bufs[c].data(),
               static_cast<std::streamsize>(out_bufs[c].size()));
  log_time("Write output text", elapsed_ms(t_w));
  return 0;
}
