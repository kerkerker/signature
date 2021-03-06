#pragma once

#include "Queues.h"

#include <filesystem>
#include <thread>

class Writer
{
public:
  Writer(IWriterQueue& queue, IExceptionsQueue& exception_queue);

  void run(std::filesystem::path const& output_file, size_t number_of_blocks, size_t hash_size);
  void join();

private:
  void write(std::filesystem::path const& output_file, size_t number_of_blocks, size_t hash_size);
  void writeToBuffer(std::filesystem::path const& output_file, size_t number_of_blocks, size_t hash_size);
  void writeToFile(std::filesystem::path const& output_file, size_t number_of_blocks, size_t hash_size);

  IWriterQueue&     queue_;
  IExceptionsQueue& exception_queue_;
  std::jthread      thread_;
};