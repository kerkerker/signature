#pragma once

#include <cstdint>
#include <memory>
#include <utility>

// using FileBlockPointer = std::pair<uint8_t *, std::size_t>; // pointer to file block, block index

struct FileBlockInfo
{
  std::unique_ptr<char[]> block;
  size_t                  index;

  bool isEmpty() const { return block == nullptr; } // TODO:? another stopper for queue
};

class IReaderQueue
{
public:
  virtual void          push(FileBlockInfo block) = 0;
  virtual FileBlockInfo pop()                     = 0;

  virtual ~IReaderQueue() = default;
};

template<typename T>
class IQueue
{
public:
  virtual void push(T&&)     = 0;
  virtual T    pop()         = 0;
  virtual bool empty() const = 0;

  virtual ~IQueue() = default;
};

using IExceptionsQueue = IQueue<std::exception_ptr>;

struct HashBlockInfo
{
  std::unique_ptr<char[]> hash;
  size_t                  index;

  bool isEmpty() const { return hash == nullptr; } // TODO
};

class IWriterQueue
{
public:
  virtual void          push(HashBlockInfo hash_block) = 0;
  virtual HashBlockInfo pop()                          = 0;

  virtual ~IWriterQueue() = default;
};

class IHasher
{
public:
  virtual std::unique_ptr<char[]> hash(std::unique_ptr<char[]> block, size_t size) const = 0;
  virtual size_t                  size() const                                           = 0;

  virtual ~IHasher() = default;
};