#include "BenchmarkUtils.hpp"
#include "ParallelSchedule.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

int main(int argc, char **argv) {
  std::string instance;
  u64 num_target_words;
  u32 batch_size;
  parseArgs(argc, argv, "server_encrypted_compute", instance, num_target_words,
            batch_size);

  const auto params = getParamsSet(instance);
  u32 num_words = computeNumWords(params);
  u32 num_cts = computeNumCiphertexts(num_target_words, num_words, batch_size);

  auto io = ioDir(instance);

  // Step timings reported back to the harness via server_reported_steps.json
  std::vector<std::pair<std::string, double>> reported_steps;
  auto report_step = [&](const char *label, double ms) {
    log_time(label, ms);
    reported_steps.emplace_back(label, ms / 1000.0);
  };

  auto t_read_key = Clock::now();
  auto relin_key = serial::loadAsPtr<ISwKey>(
      (io / "public_keys" / "relin_key.bin").string());
  double t_read_key_ms = elapsed_ms(t_read_key);
  report_step("Read relinearization key", t_read_key_ms);

  auto t_setup = Clock::now();
  auto t_ptxt =
      serial::loadAsPtr<IPlaintext>((io / "context" / "t_ptxt.bin").string());
  auto rescale_mod = params.eval_params.getLevels().mods[0];
  double t_setup_ms = elapsed_ms(t_setup);
  report_step("Setup", t_setup_ms);

  ensureDir(io / "ciphertexts_download");

  auto process_ct = [&](u32 c, HomEval &eval, HomEvalFlexible &eval_flex) {
    auto ctxt0 = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_upload" / ("ctxt0_" + std::to_string(c) + ".bin"))
            .string());
    auto ctxt1 = serial::loadAsPtr<ICiphertext>(
        (io / "ciphertexts_upload" / ("ctxt1_" + std::to_string(c) + ".bin"))
            .string());
    auto res = ICiphertext::make(getEncType(batch_size));
    eval.tensor(*ctxt0, *ctxt1, *res);
    eval.relin(*res, *relin_key);
    eval.mul(*res, *t_ptxt, *res);
    eval_flex.rescale(*res, *res, rescale_mod);
    serial::save(
        (io / "ciphertexts_download" / ("res_" + std::to_string(c) + ".bin"))
            .string(),
        *res);
  };

  struct Engines {
    HomEval eval;
    HomEvalFlexible eval_flex;
  };
  auto t_compute = Clock::now();
  runFheTasks(
      num_cts, outerThreads(num_cts),
      [&] {
        return Engines{HomEval(params.eval_params), HomEvalFlexible{}};
      },
      [&](u32 c, Engines &e) { process_ct(c, e.eval, e.eval_flex); });
  double t_compute_ms = elapsed_ms(t_compute);
  report_step("Encrypted computation", t_compute_ms);
  report_step("Total", t_read_key_ms + t_setup_ms + t_compute_ms);

  // Write the step timings (in seconds) for the harness to pick up.
  std::ofstream report(io / "server_reported_steps.json");
  report << std::fixed << std::setprecision(4) << "{\n";
  for (size_t i = 0; i < reported_steps.size(); ++i) {
    report << "  \"" << reported_steps[i].first
           << "\": " << reported_steps[i].second
           << (i + 1 < reported_steps.size() ? ",\n" : "\n");
  }
  report << "}\n";
  return 0;
}
