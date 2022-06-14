#include "Writer.h"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

Writer::Writer(IWriterQueue& queue, IExceptionsQueue& exception_queue)
    : queue_{queue},
      exception_queue_{exception_queue}
{}

void Writer::run(const std::filesystem::path& output_file, size_t number_of_blocks, size_t hash_size)
{
  spdlog::debug("number of hash blocks: {}, hash size: {}", number_of_blocks, hash_size);

  constexpr size_t kLimit{1'000'000'000};
  if (number_of_blocks * hash_size < kLimit) {
    thread_ = std::jthread(&Writer::readToBuffer, this, output_file, number_of_blocks, hash_size);
  } else {
    thread_ = std::jthread(&Writer::readToFile, this, output_file, number_of_blocks, hash_size);
  }
}

void Writer::readToBuffer(std::filesystem::path const& output_file, size_t number_of_blocks, size_t hash_size)
{
  spdlog::debug("writing hash to buffer");

  std::vector<char> buf;
  auto              buf_size = number_of_blocks == 0 ? hash_size : number_of_blocks * hash_size;
  buf.resize(buf_size);

  for (auto hash_info = queue_.pop(); !hash_info.isEmpty(); hash_info = queue_.pop()) {
    std::memcpy(buf.data() + hash_info.index * hash_size, hash_info.hash.get(), hash_size);
  }

  std::ofstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    file.open(output_file, std::ios::binary | std::ios::out);
    file.write(&buf.front(), static_cast<std::streamsize>(buf.size()));
  } catch (const std::ios_base::failure& ex) {
    spdlog::error(
        "Caught an ios_base::failure.\n"
        "Error code: {} ( {} ) Error category: {}\n{}\noutput file: {}",
        ex.code().value(),
        ex.code().message(),
        ex.code().category().name(),
        std::strerror(errno),
        output_file.string());
    exception_queue_.push(std::current_exception());
  }
}

void Writer::readToFile(const std::filesystem::path& output_file, size_t number_of_blocks, size_t hash_size)
{
  spdlog::debug("writing hash to file directly");

  std::ofstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    file.open(output_file, std::ios::binary | std::ios::out);
    file.seekp(static_cast<std::streamsize>(number_of_blocks * hash_size));
    for (auto hash_info = queue_.pop(); !hash_info.isEmpty(); hash_info = queue_.pop()) {
      file.seekp(static_cast<std::streamsize>(hash_info.index * hash_size), std::ios::beg);
      file.write(hash_info.hash.get(), static_cast<std::streamsize>(hash_size));
    }
  } catch (const std::ios_base::failure& ex) {
    spdlog::error(
        "Caught an ios_base::failure.\n"
        "Error code: {} ( {} ) Error category: {}\n{}\noutput file: {}",
        ex.code().value(),
        ex.code().message(),
        ex.code().category().name(),
        std::strerror(errno),
        output_file.string());
    exception_queue_.push(std::current_exception());
  }
}

void Writer::join()
{

  if (thread_.joinable()) {
    thread_.join();
  }
}
