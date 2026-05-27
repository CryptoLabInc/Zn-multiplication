#include "BenchmarkUtils.hpp"
#include "WordEncoder.hpp"

#include <iostream>

int main(int argc, char **argv) {
  std::string instance;
  u64 num_target_words;
  u32 batch_size;
  parseArgs(argc, argv, "server_encrypted_compute", instance, num_target_words,
            batch_size);

  const auto params = getParamsSet();
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);

  auto io = ioDir(instance);

  auto t_read_key = Clock::now();
  auto relin_key = serial::loadAsPtr<ISwKey>(
      (io / "public_keys" / "relin_key.bin").string());
  log_time("read relin key", elapsed_ms(t_read_key));

  auto t_setup = Clock::now();
  // Build the fixed reduction plaintext t(X) = -2 + X at each word slot.
  Message t(log_degree);
  for (size_t w = 0; w < num_words; ++w) {
    size_t base = w * BIT_WIDTH;
    t[base] = -2.0;
    t[base + 1] = 1.0;
  }

  WordEncodeParams word_ecd_params;
  word_ecd_params.setLogDegree(log_degree);
  word_ecd_params.setBatchSize(batch_size);
  WordEncoder word_encoder(word_ecd_params);

  Message t_msg;
  word_encoder.arithToComplex(t, t_msg);

  EnDecoder encoder(params.ecd_params);
  auto t_ptxt = IPlaintext::make(getPtxtType(batch_size));
  std::vector<Message> t_msgs;
  t_msgs.reserve(batch_size);
  for (u32 i = 0; i < batch_size; ++i)
    t_msgs.push_back(t_msg.copy());
  encodeBatch(encoder, t_msgs, *t_ptxt, 2);

  HomEval eval(params.eval_params);
  HomEvalFlexible eval_flex;
  log_time("setup (plaintext/eval)", elapsed_ms(t_setup));

  ensureDir(io / "ciphertexts_download");

  double total_read_ms = 0, total_compute_ms = 0, total_write_ms = 0;

  for (u32 c = 0; c < num_cts; ++c) {
    auto t_rd = Clock::now();
    auto ctxt0 = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_upload" / ("ctxt0_" + std::to_string(c) + ".bin"))
            .string());
    auto ctxt1 = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_upload" / ("ctxt1_" + std::to_string(c) + ".bin"))
            .string());
    total_read_ms += elapsed_ms(t_rd);

    auto t_comp = Clock::now();
    auto res = ICiphertext::make(getEncType(batch_size));
    eval.tensor(*ctxt0, *ctxt1, *res);
    eval.relin(*res, *relin_key);
    eval.mul(*res, *t_ptxt, *res);
    eval_flex.rescale(*res, *res, params.eval_params.getLevels().mods[0]);
    total_compute_ms += elapsed_ms(t_comp);

    auto t_wr = Clock::now();
    serial::save(
        (io / "ciphertexts_download" / ("res_" + std::to_string(c) + ".bin"))
            .string(),
        *res);
    total_write_ms += elapsed_ms(t_wr);
  }

  log_time("read ciphertexts", total_read_ms);
  log_time("compute (tensor+relin+mul+rescale)", total_compute_ms);
  log_time("write result ciphertexts", total_write_ms);
  return 0;
}
