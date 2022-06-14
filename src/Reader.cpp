#include "Reader.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

static std::pair<size_t, size_t> get_file_blocks_info(std::filesystem::path const& input_file, size_t block_size)
{
  auto file_size        = std::filesystem::file_size(input_file);
  auto number_of_blocks = file_size / block_size;
  auto last_block_size  = file_size % block_size;

  if (last_block_size != 0) {
    ++number_of_blocks;
  }

  if (last_block_size == 0) {
    last_block_size = block_size;
  }

  return {number_of_blocks, last_block_size};
}

size_t get_number_of_blocks(std::filesystem::path const& input_file, size_t block_size)
{
  return get_file_blocks_info(input_file, block_size).first;
}

static std::unique_ptr<char[]> try_alloc_buffer(size_t block_size, size_t number_of_tries = 10)
{
  using namespace std::chrono_literals;

  do {
    try {
      return std::make_unique_for_overwrite<char[]>(block_size);
    } catch (std::bad_alloc const& ex) {
      spdlog::debug(ex.what());
      --number_of_tries;
      std::this_thread::sleep_for(1000ms);

      if (number_of_tries == 0) {
        throw;
      }
    }

  } while (true);
}

Reader::Reader(IReaderQueue& queue, IExceptionsQueue& exceptions_queue)
    : queue_{queue},
      exceptions_queue_{exceptions_queue}
{}

void Reader::run(std::filesystem::path const& input_file, size_t block_size)
{
  thread_ = std::jthread(&Reader::readFile, this, input_file, block_size);
}

void Reader::readFile(std::filesystem::path const& input_file, size_t block_size)
{
  try {
    readFileImpl(input_file, block_size);
  } catch (std::bad_alloc const& ex) {
    spdlog::error("{}, try a smaller block size (current is {})", ex.what(), block_size);
    exceptions_queue_.push(std::current_exception());
  } catch (std::exception const& ex) {
    spdlog::error(ex.what());
    exceptions_queue_.push(std::current_exception());
  }

  queue_.push(FileBlockInfo::Stopper());
}

void Reader::readFileImpl(std::filesystem::path const& input_file, size_t block_size)
{
  auto [number_of_blocks, last_block_size] = get_file_blocks_info(input_file, block_size);

  spdlog::debug("number of blocks: {}, last block size: {}", number_of_blocks, last_block_size);

  std::ifstream ifs{input_file, std::ifstream::binary};

  for (size_t i = 0; i < number_of_blocks; ++i) {
    auto block = try_alloc_buffer(block_size);

    auto cur_block_size = i == number_of_blocks - 1 && i != 0 ? block_size : last_block_size;
    ifs.read(block.get(), static_cast<std::streamsize>(cur_block_size));

    if (cur_block_size < block_size) {
      std::fill_n(block.get() + cur_block_size, block_size - cur_block_size, 0);
    }

    spdlog::trace("[{}/{}] block read", i, number_of_blocks);

    queue_.push({std::move(block), i});
  }

  if (number_of_blocks == 0) {
    queue_.push({std::make_unique<char[]>(block_size), 0});
  }
}
