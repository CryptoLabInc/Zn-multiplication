#include "BenchmarkUtils.hpp"
#include "WordEncoder.hpp"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: client_key_generation <instance>" << std::endl;
    return 1;
  }
  std::string instance = argv[1];

  const auto params = getParamsSet(instance);
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 num_words = computeNumWords(params);
  u32 batch_size = defaultBatchSizeFromInstance(instance);

  auto io = ioDir(instance);
  ensureDir(io / "secret_keys");
  ensureDir(io / "public_keys");
  ensureDir(io / "context");

  auto t_pt = Clock::now();
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
  serial::save((io / "context" / "t_ptxt.bin").string(), *t_ptxt);
  log_time("Reduction plaintext", elapsed_ms(t_pt));

  auto t0 = Clock::now();
  auto sk = SKGenerator(params.skgen_params).genKey();
  auto relin_key = SwKeyGenerator(params.swkgen_params).genRelinKey(*sk);
  log_time("Keygen compute", elapsed_ms(t0));

  auto t1 = Clock::now();
  serial::save((io / "secret_keys" / "sk.bin").string(), *sk);
  serial::save((io / "public_keys" / "relin_key.bin").string(), *relin_key);
  log_time("Keygen write", elapsed_ms(t1));

  return 0;
}
