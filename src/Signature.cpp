#include "Signature.h"

#include "Calculator.h"
#include "Reader.h"

#include <openssl/sha.h>

#include <cstring>
#include <iostream>

static std::unique_ptr<IHasher> get_hasher(HashAlgorithm algorithm)
{
  switch (algorithm) {
    case HashAlgorithm::kSha1: return std::make_unique<Sha1Hasher>();
    case HashAlgorithm::kCrc32: return std::make_unique<Crc32Hasher>();
  }

  throw std::invalid_argument("Unknown hash algorithm");
}

FileSignatureGenerator::FileSignatureGenerator(HashAlgorithm algorithm)
    : hasher_{get_hasher(algorithm)},
      reader_{reader_queue_, exceptions_queue_},
      calculator_{reader_queue_, writer_queue_, exceptions_queue_, *hasher_},
      writer_{writer_queue_, exceptions_queue_}
{}

void FileSignatureGenerator::run(
    const std::filesystem::path& input_file,
    const std::filesystem::path& output_file,
    size_t                       block_size)
{
  const auto number_of_blocks = get_number_of_blocks(input_file, block_size);

  reader_.run(input_file, block_size);
  writer_.run(output_file, number_of_blocks, hasher_->size());
  calculator_.run(block_size);
  writer_.join();
  while (!exceptions_queue_.empty()) {
    auto ptr = exceptions_queue_.pop();
    std::rethrow_exception(ptr);
  }
}

std::unique_ptr<char[]> Sha1Hasher::hash(std::unique_ptr<char[]> block, size_t size) const
{
  auto hash_block = std::make_unique_for_overwrite<char[]>(kDigestSize);
  SHA1(reinterpret_cast<unsigned char*>(block.get()), size, reinterpret_cast<unsigned char*>(hash_block.get()));

  return hash_block;
}

size_t Sha1Hasher::size() const { return kDigestSize; }

std::unique_ptr<char[]> Crc32Hasher::hash(std::unique_ptr<char[]> block, size_t size) const
{
  boost::crc_32_type result;
  result.process_bytes(block.get(), size);

  auto     hash_block = std::make_unique_for_overwrite<char[]>(kDigestSize);
  uint32_t checksum   = result.checksum();
  std::memcpy(hash_block.get(), &checksum, sizeof(checksum));

  return hash_block;
}

size_t Crc32Hasher::size() const { return kDigestSize; }
