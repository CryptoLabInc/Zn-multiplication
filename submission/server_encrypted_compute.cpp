#include "BenchmarkUtils.hpp"
#include "ParallelSchedule.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char **argv) {
  const auto t_server = Clock::now();

  // Server initialization: arguments, parameters and output directory.
  auto t_step = Clock::now();
  std::string instance;
  u64 num_target_words;
  u32 batch_size;
  parseArgs(argc, argv, "server_encrypted_compute", instance, num_target_words,
            batch_size);

  const auto params = getParamsSet(instance);
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);
  auto rescale_mod = params.eval_params.getLevels().mods[0];

  auto io = ioDir(instance);
  ensureDir(io / "ciphertexts_download");
  const double t_init_ms = elapsed_ms(t_step);

  // Load the public material: the relinearization key and the plaintext
  // t(X) = -2 + X used for the word-wise reduction.
  t_step = Clock::now();
  auto relin_key = serial::loadAsPtr<ISwKey>(
      (io / "public_keys" / "relin_key.bin").string());
  auto t_ptxt =
      serial::loadAsPtr<IPlaintext>((io / "context" / "t_ptxt.bin").string());
  const double t_keys_ms = elapsed_ms(t_step);

  // Storage for ciphertext pairs and results.
  struct CtPair {
    Ptr<ICiphertext> ct0, ct1;
  };
  std::vector<CtPair> inputs(num_cts);
  std::vector<Ptr<ICiphertext>> results(num_cts);

  // Read all inputs.
  t_step = Clock::now();
#pragma omp parallel for schedule(dynamic) num_threads(outerThreads(num_cts))
  for (u32 c = 0; c < num_cts; ++c) {
    inputs[c].ct0 = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_upload" / ("ctxt0_" + std::to_string(c) + ".bin"))
            .string());
    inputs[c].ct1 = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_upload" / ("ctxt1_" + std::to_string(c) + ".bin"))
            .string());
  }
  const double t_read_ms = elapsed_ms(t_step);

  // Encrypted computation.
  t_step = Clock::now();
  runFheTasks(
      num_cts, outerThreads(num_cts),
      [&] {
        struct Engines {
          HomEval eval;
          HomEvalFlexible eval_flex;
        };
        return Engines{HomEval(params.eval_params), HomEvalFlexible{}};
      },
      [&](u32 c, auto &e) {
        auto res = ICiphertext::make(getEncType(batch_size));
        e.eval.tensor(*inputs[c].ct0, *inputs[c].ct1, *res);
        e.eval.relin(*res, *relin_key);
        e.eval.mul(*res, *t_ptxt, *res);
        e.eval_flex.rescale(*res, *res, rescale_mod);
        results[c] = std::move(res);
      });
  const double t_compute_ms = elapsed_ms(t_step);

  // Write all outputs.
  t_step = Clock::now();
#pragma omp parallel for schedule(dynamic) num_threads(outerThreads(num_cts))
  for (u32 c = 0; c < num_cts; ++c) {
    serial::save(
        (io / "ciphertexts_download" / ("res_" + std::to_string(c) + ".bin"))
            .string(),
        *results[c]);
  }
  const double t_write_ms = elapsed_ms(t_step);

  std::vector<std::pair<std::string, double>> reported_steps;
  auto report_step = [&](const char *label, double ms) {
    log_time(label, ms);
    reported_steps.emplace_back(label, ms / 1000.0);
  };

  report_step("Server initialization", t_init_ms);
  report_step("Load public keys", t_keys_ms);
  report_step("Read inputs", t_read_ms);
  report_step("Encrypted computation", t_compute_ms);
  report_step("Write outputs", t_write_ms);
  report_step("Total", elapsed_ms(t_server));

  // Write the step timings (in seconds) for the harness to pick up.
  std::ofstream report(io / "server_reported_steps.json");
  report << std::fixed << std::setprecision(5) << "{\n";
  for (size_t i = 0; i < reported_steps.size(); ++i) {
    report << "  \"" << reported_steps[i].first << "\": \""
           << reported_steps[i].second << "\""
           << (i + 1 < reported_steps.size() ? ",\n" : "\n");
  }
  report << "}\n";
  return 0;
}
