#pragma once

#include "Queues.h"

#include <filesystem>
#include <thread>

class Reader
{
public:
  explicit Reader(IReaderQueue& queue, IExceptionsQueue& exceptions_queue);

  void run(std::filesystem::path input_file, size_t block_size);

private:
  void readFile(std::filesystem::path input_file, size_t block_size);

  IReaderQueue&     queue_;
  IExceptionsQueue& exceptions_queue_;
  std::jthread      thread_;
};

size_t get_number_of_blocks(std::filesystem::path const& input_file, size_t block_size);