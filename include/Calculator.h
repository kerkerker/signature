#pragma once

#include "Queues.h"

class Calculator
{
public:
  Calculator(
      IReaderQueue&     reader_queue,
      IWriterQueue&     writer_queue,
      IExceptionsQueue& exception_queue,
      IHasher&          hasher);

  void run(size_t block_size);

private:
  IReaderQueue&     reader_queue_;
  IWriterQueue&     writer_queue_;
  IExceptionsQueue& exception_queue_;
  IHasher&          hasher_;
};