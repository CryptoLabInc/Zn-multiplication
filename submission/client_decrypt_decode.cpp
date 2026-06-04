#include "BenchmarkUtils.hpp"
#include "WordEncoder.hpp"

int main(int argc, char **argv) {
  std::string instance;
  u64 num_target_words;
  u32 batch_size;
  parseArgs(argc, argv, "client_decrypt_decode", instance, num_target_words,
            batch_size);

  const auto params = getParamsSet();
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);

  auto io = ioDir(instance);

  auto t_read_sk = Clock::now();
  auto sk =
      serial::loadAsPtr<ISecretKey>((io / "secret_keys" / "sk.bin").string());
  log_time("Read secret key", elapsed_ms(t_read_sk));

  auto t_setup = Clock::now();
  EnDecryptor encryptor(params.enc_params);
  EnDecoder encoder(params.ecd_params);

  WordEncodeParams word_ecd_params;
  word_ecd_params.setLogDegree(log_degree);
  word_ecd_params.setBatchSize(batch_size);
  WordEncoder word_encoder(word_ecd_params);
  log_time("Setup", elapsed_ms(t_setup));

  auto out_dir = io / "cleartext_output";
  ensureDir(out_dir);
  std::ofstream os(out_dir / "out.txt");
  if (!os)
    throw std::runtime_error("cannot write cleartext output");

  double total_read_ms = 0, total_decrypt_ms = 0, total_decode_ms = 0,
         total_write_ms = 0;

  u64 emitted = 0;
  for (u32 c = 0; c < num_cts && emitted < num_target_words; ++c) {
    auto t_rd = Clock::now();
    auto ctxt = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_download" / ("res_" + std::to_string(c) + ".bin"))
            .string());
    total_read_ms += elapsed_ms(t_rd);

    auto t_dec = Clock::now();
    auto ptxt = IPlaintext::make(getPtxtType(batch_size));
    encryptor.decrypt(*ctxt, *sk, *ptxt);
    total_decrypt_ms += elapsed_ms(t_dec);

    auto t_decode = Clock::now();
    std::vector<Message> msgs;
    decodeBatch(encoder, *ptxt, msgs, batch_size);

    std::vector<std::vector<u64>> words;
    word_encoder.complexToWord(msgs, words);
    total_decode_ms += elapsed_ms(t_decode);

    auto t_wr = Clock::now();
    for (u32 b = 0; b < batch_size && emitted < num_target_words; ++b) {
      for (u32 w = 0; w < num_words && emitted < num_target_words; ++w) {
        os << words[b][w] << "\n";
        ++emitted;
      }
    }
    total_write_ms += elapsed_ms(t_wr);
  }

  log_time("Read result ciphertexts", total_read_ms);
  log_time("Decrypt", total_decrypt_ms);
  log_time("Decode", total_decode_ms);
  log_time("Write output text", total_write_ms);
  return 0;
}
