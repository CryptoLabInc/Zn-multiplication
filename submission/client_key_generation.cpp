#include "BenchmarkUtils.hpp"

#include <iostream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: client_key_generation <instance>" << std::endl;
    return 1;
  }
  std::string instance = argv[1];

  auto t0 = Clock::now();
  const auto params = getParamsSet(instance);

  auto sk = SKGenerator(params.skgen_params).genKey();

  auto relin_key = SwKeyGenerator(params.swkgen_params).genRelinKey(*sk);
  log_time("Keygen compute", elapsed_ms(t0));

  auto io = ioDir(instance);
  ensureDir(io / "secret_keys");
  ensureDir(io / "public_keys");

  auto t1 = Clock::now();
  serial::save((io / "secret_keys" / "sk.bin").string(), *sk);
  serial::save((io / "public_keys" / "relin_key.bin").string(), *relin_key);
  log_time("Keygen write", elapsed_ms(t1));

  return 0;
}
