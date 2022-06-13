#include "Calculator.h"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <iostream>
#include <thread>

namespace asio = boost::asio;

Calculator::Calculator(
    IReaderQueue&     reader_queue,
    IWriterQueue&     writer_queue,
    IExceptionsQueue& exception_queue,
    IHasher&          hasher)
    : reader_queue_{reader_queue},
      writer_queue_{writer_queue},
      exception_queue_{exception_queue},
      hasher_{hasher}
{}

void Calculator::run(size_t block_size)
{
  auto core_number = std::thread::hardware_concurrency() * 2; // TODO: hyperthreading? win/linux native functions
  asio::thread_pool thread_pool{core_number};

  for (auto block_info = reader_queue_.pop(); !block_info.isEmpty(); block_info = reader_queue_.pop()) {
    asio::post(
        thread_pool,
        [&hasher       = hasher_,
         &writer_queue = writer_queue_,
         block_info    = std::move(block_info),
         block_size,
         &exception_queue = exception_queue_]() mutable {
         // try {
            auto hash_block = hasher.hash(std::move(block_info.block), block_size);
            writer_queue.push({std::move(hash_block), block_info.index});
//          } catch (std::exception const&) {
//            exception_queue.push(std::current_exception());
//          }
        });
  }

  thread_pool.wait();
  writer_queue_.push({nullptr, 0});
}
