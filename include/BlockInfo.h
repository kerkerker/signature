#pragma once

#include <memory>

struct FileBlockInfo
{
  std::unique_ptr<char[]> block;
  size_t                  index;

  static FileBlockInfo Stopper() { return FileBlockInfo{nullptr, 0}; }

  bool isStopper() const { return *this == Stopper(); }

  friend auto operator<=>(FileBlockInfo const&, FileBlockInfo const&) = default;
};

struct HashBlockInfo
{
  std::unique_ptr<char[]> hash;
  size_t                  index;

  static HashBlockInfo Stopper() { return HashBlockInfo{nullptr, 0}; }

  bool isStopper() const { return *this == Stopper(); }

  friend auto operator<=>(HashBlockInfo const&, HashBlockInfo const&) = default;
};