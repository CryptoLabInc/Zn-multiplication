#pragma once

#include "HEaaN2/HEaaN2.hpp"

using namespace heaan;

struct WordEncodeParams {
  WordEncodeParams &setLogDegree(u32 log_degree);
  WordEncodeParams &setBatchSize(u32 batch_size);

  u32 log_degree_;
  u32 batch_size_ = 1;
};

class WordEncoder {
public:
  WordEncoder();
  WordEncoder(const WordEncodeParams &params);

  // Convert a vector of u64 words to an arithmetic message.
  // For each word m in Z_{2^64}, it converts m to (1/t)*m
  void wordToArith(const std::vector<u64> &words, Message &arith) const;

  // Convert an arithmetic message to a vector of u64 words.
  // This is the inverse of wordToArith.
  void arithToWord(const Message &arith, std::vector<u64> &words) const;

  // Convert an arithmetic message to a complex message.
  // This corresponds to the isomorphism
  // R[X]/(X^64 - X + 2) -> C^{32}
  void arithToComplex(const Message &arith, Message &msg) const;

  // Convert a complex message to an arithmetic message.
  // This is the inverse of arithToComplex.
  void complexToArith(const Message &msg, Message &arith) const;

  // wordToArith + arithToComplex
  // This avoids explicitly constructing the intermediate arithmetic message,
  // and directly computes the complex message from the input words.
  void wordToComplex(const std::vector<std::vector<u64>> &words,
                     std::vector<Message> &msgs) const;

  // complexToArith + arithToWord
  // This avoids explicitly constructing the intermediate arithmetic message,
  // and directly computes the output words from the input complex message.
  void complexToWord(const std::vector<Message> &msgs,
                     std::vector<std::vector<u64>> &words) const;

  const std::optional<WordEncodeParams> &getParams() const { return params_; }

private:
  const std::optional<WordEncodeParams> params_ = std::nullopt;
};
