#include "Signature.h"

#include "Calculator.h"
#include "Reader.h"
#include "Writer.h"

#include <boost/lockfree/queue.hpp>
#include <openssl/sha.h>

#include <iostream>

namespace fs = std::filesystem;

std::unique_ptr<char[]> Sha1Hasher::hash(std::unique_ptr<char[]> block, size_t size) const
{
  auto hash_block = std::make_unique_for_overwrite<char[]>(kDigestSize);
  SHA1(reinterpret_cast<unsigned char*>(block.get()), size, reinterpret_cast<unsigned char*>(hash_block.get()));

  //  boost::uuids::detail::sha1 sha1_;
  //  sha1_.process_bytes(block.get(), size);
  //  thread_local boost::uuids::detail::sha1::digest_type digest;
  //  sha1_.get_digest(digest);
  //
  //  auto hash_block = std::make_unique_for_overwrite<char[]>(kDigestSize);
  //  std::memcpy(hash_block.get(), digest, kDigestSize);

  //  sha1_.reset();

  return hash_block;
}

size_t Sha1Hasher::size() const { return kDigestSize; }

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

FileSignatureGenerator::FileSignatureGenerator(HashAlgorithm algorithm)
    : hasher_{get_hasher(algorithm)},
      reader_{reader_queue_, exceptions_queue_},
      calculator_{reader_queue_, writer_queue_, exceptions_queue_, *hasher_},
      writer_{writer_queue_, exceptions_queue_}
{}

std::unique_ptr<char[]> Crc32Hasher::hash(std::unique_ptr<char[]> block, size_t size) const
{
  boost::crc_32_type result;
  result.process_bytes(block.get(), size);

  auto     hash_block = std::make_unique_for_overwrite<char[]>(kDigestSize);
  uint32_t checksum   = result.checksum();
  std::memcpy(hash_block.get(), &checksum, sizeof(checksum));

  //  *reinterpret_cast<uint32_t*>(hash_block.get()) = checksum;
  return hash_block;
}

size_t Crc32Hasher::size() const { return kDigestSize; }
