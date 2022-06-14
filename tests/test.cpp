#include "Signature.h"

#include <gtest/gtest.h>
#include <openssl/sha.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>

namespace fs = std::filesystem;

std::string to_hex(std::string const& str)
{
  std::stringstream ss;
  for (auto c : str) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<uint8_t>(c));
  }

  return ss.str();
}

std::string read_file(fs::path const& file)
{
  if (std::ifstream is{file, std::ios::binary | std::ios::ate}) {
    auto        size = is.tellg();
    std::string str(size, '\0');
    is.seekg(0);
    is.read(&str[0], size);
    return str;
  }

  return {};
}

std::string read_signature(fs::path const& signature_file) { return to_hex(read_file(signature_file)); }

std::string multiply(std::string const& str, size_t n)
{
  std::string result;
  result.reserve(str.size() * n);

  while (n--) {
    result.append(str);
  }

  return result;
}

std::string switch_crc_endian(std::string signature)
{
  constexpr size_t kCrc32Size{4};

  for (auto i = signature.begin(); i < signature.end(); i += kCrc32Size) {
    std::reverse(i, std::next(i, kCrc32Size));
  }

  return to_hex(signature);
}

std::string read_crc32_signature(fs::path const& file)
{
  auto signature = read_file(file);
  return switch_crc_endian(signature);
}

TEST(Signature, EmptyFileSha1)
{
  auto output = fs::path{"empty_signature.bin"};

  FileSignatureGenerator signature_generator{HashAlgorithm::kSha1};

  signature_generator.run("empty.bin", output, 64);

  auto signature = read_signature(output);
  ASSERT_EQ(signature, "c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7");
}

TEST(Signature, EmptyFileCrc32)
{
  FileSignatureGenerator signature_generator{HashAlgorithm::kCrc32};
  auto                   output = fs::path{"in_big_32.bin"};
  signature_generator.run("empty.bin", output, 64);
  auto signature = read_crc32_signature(output);
  ASSERT_EQ(signature, "758d6336");
}

TEST(Signature, FileCrc32)
{
  FileSignatureGenerator signature_generator{HashAlgorithm::kCrc32};
  auto                   output = fs::path{"empty_signature_32.bin"};
  signature_generator.run("in_big.bin", output, 64000);
  auto signature = read_crc32_signature(output);
  ASSERT_EQ(signature, "35a6418a35a6418a35a6418a35a6418a35a6418a35a6418a");
}

TEST(Signature, Zero1kbFile)
{
  FileSignatureGenerator signature_generator{HashAlgorithm::kSha1};
  auto                   output = fs::path{"empty_signature.bin"};

  constexpr size_t kFileSize{1024};
  constexpr size_t kBlockSize{64};
  constexpr size_t kNumberOfBlocks{kFileSize / kBlockSize};

  signature_generator.run("nullbytes.bin", output, kBlockSize);
  auto signature = read_signature(output);

  ASSERT_EQ(signature, multiply("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7", kNumberOfBlocks));
}

TEST(Signature, MultipleFilesRun)
{
  FileSignatureGenerator signature_generator{HashAlgorithm::kSha1};

  const auto first_input   = fs::path{"empty.bin"};
  const auto first_output  = fs::path{"empty_signature.bin"};
  const auto second_input  = fs::path{"nullbytes.bin"};
  const auto second_output = fs::path{"nullbytes_signature.bin"};

  constexpr size_t kFileSize{1024};
  constexpr size_t kBlockSize{64};
  constexpr size_t kNumberOfBlocks{kFileSize / kBlockSize};

  signature_generator.run(first_input, first_output, 64);
  auto first_signature = read_signature(first_output);
  ASSERT_EQ(first_signature, "c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7");

  signature_generator.run(second_input, second_output, 64);

  auto second_signature = read_signature(second_output);
  ASSERT_EQ(second_signature, multiply("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7", kNumberOfBlocks));
}

TEST(Signature, NonExistingInputFile)
{
  FileSignatureGenerator signature_generator;
  auto                   output = fs::path{"out.bin"};

  ASSERT_THROW(
      {
        try {
          signature_generator.run("thisfiledoesnotexist.bin", output, 64);
        } catch (std::filesystem::filesystem_error const& e) {
          std::string expected{"No such file or directory"};
          EXPECT_TRUE(std::string{e.what()}.find(expected) != std::string::npos);
          throw;
        }
      },
      std::filesystem::filesystem_error);
}

TEST(Signature, NonExistingOutputDir)
{
  FileSignatureGenerator signature_generator;
  auto                   output = fs::path{"/this/dir/does/not/exits.bin"};

  ASSERT_THROW(
      {
        try {
          signature_generator.run("empty_signature.bin", output, 64);
        } catch (std::ios_base::failure const& e) {
          EXPECT_STREQ("basic_ios::clear: iostream error", e.what());
          throw;
        }
      },
      std::ios_base::failure);
}
