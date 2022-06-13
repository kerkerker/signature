#pragma once

#include "Calculator.h"
#include "ConcurrentQueue.h"
#include "Queues.h"
#include "Reader.h"
#include "Writer.h"

#include <boost/compute/detail/sha1.hpp>
#include <boost/crc.hpp>

#include <filesystem>

class ReaderQueue : public IReaderQueue
{
public:
  void          push(FileBlockInfo block) override { queue_.push(std::move(block)); }
  FileBlockInfo pop() override { return queue_.pop(); }

private:
  ConcurrentQueue<FileBlockInfo> queue_;
};

class WriterQueue : public IWriterQueue
{
public:
  void          push(HashBlockInfo block) override { queue_.push(std::move(block)); }
  HashBlockInfo pop() override { return queue_.pop(); }

private:
  ConcurrentQueue<HashBlockInfo> queue_;
};

class ExceptionsQueue : public IExceptionsQueue
{
public:
  void               push(std::exception_ptr&& ptr) override { queue_.push(std::move(ptr)); }
  std::exception_ptr pop() override { return queue_.pop(); }

  bool empty() const override { return queue_.empty(); }

private:
  ConcurrentQueue<std::exception_ptr> queue_;
};

class Sha1Hasher : public IHasher
{
public:
  std::unique_ptr<char[]> hash(std::unique_ptr<char[]> block, size_t size) const override;
  size_t                  size() const override;

private:
  static constexpr size_t kDigestSize{sizeof(boost::uuids::detail::sha1::digest_type)};
};

class Crc32Hasher : public IHasher
{
public:
  std::unique_ptr<char[]> hash(std::unique_ptr<char[]> block, size_t size) const override;
  size_t                  size() const override;

private:
  static constexpr size_t kDigestSize{4};
};

enum class HashAlgorithm
{
  kSha1,
  kCrc32
};

inline std::unique_ptr<IHasher> get_hasher(HashAlgorithm algorithm)
{
  switch (algorithm) {
    case HashAlgorithm::kSha1: return std::make_unique<Sha1Hasher>();
    case HashAlgorithm::kCrc32: return std::make_unique<Crc32Hasher>();
  }

  throw std::invalid_argument("Unknown hash algorithm");
}

class FileSignatureGenerator
{
public:
  explicit FileSignatureGenerator(HashAlgorithm algorithm = HashAlgorithm::kSha1);

  void run(std::filesystem::path const& input_file, std::filesystem::path const& output_file, size_t block_size);

private:
  std::unique_ptr<IHasher> hasher_;
  ExceptionsQueue          exceptions_queue_;
  ReaderQueue              reader_queue_;
  WriterQueue              writer_queue_;
  Reader                   reader_;
  Calculator               calculator_;
  Writer                   writer_;
};