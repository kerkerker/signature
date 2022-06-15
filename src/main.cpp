#include "Signature.h"

#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

#include <concepts>
#include <filesystem>
#include <iostream>

#ifdef __linux__
#include <sys/resource.h>
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;

template<typename F, typename... Args>
  requires std::is_invocable_v<F, Args...>
void measure_time(F&& f, Args... args)
{
  auto begin = std::chrono::steady_clock::now();
  std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  auto end = std::chrono::steady_clock::now();

  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
  spdlog::info("Elapsed time: {} [s]", elapsed);
}

void set_log_level(std::string const& level)
{
  using level_enum = spdlog::level::level_enum;

  static const std::unordered_map<std::string, level_enum> kStrToLog = {
      {"trace", level_enum::trace},
      {"trace", level_enum::debug},
      {"trace", level_enum::info}};

  auto log_level = spdlog::level::info;

  try {
    log_level = kStrToLog.at(level);
  } catch (std::out_of_range const&) {
    spdlog::set_level(spdlog::level::info);
  }
}

int main(int argc, const char* argv[])
{
  po::options_description opt_desc{"Options"};

  auto block_size_check = [](std::size_t value) {
    if (value <= 0) {
      throw po::validation_error(po::validation_error::invalid_option_value, "block_size", std::to_string(value));
    }
  };

  constexpr size_t kDefaultBlockSize{1000000};
  // clang-format off
  opt_desc.add_options()
    ("help,h", "Display this information.")
    ("in_file,i", po::value<fs::path>()->required(), "Path to the input file")
    ("out_file,o", po::value<fs::path>()->required(), "Path to the output file")
    ("block_size,b", po::value<std::size_t>()->default_value(kDefaultBlockSize)->notifier(block_size_check),
      "Block size for hashing (>0)")
#ifdef __linux__
    ("memlimit,m", po::value<rlim_t>(), "Set memory limit (for linux only)")
#endif
    ("log,l", po::value<std::string>()->default_value("info"), "Log level (trace, debug, info)");
  // clang-format on

  po::variables_map vm;
  try {
    po::store(parse_command_line(argc, argv, opt_desc), vm);
    po::notify(vm);
  } catch (boost::program_options::error const& ex) {
    spdlog::error(ex.what());
    return 1;
  }

  set_log_level(vm["log"].as<std::string>());

#ifdef __linux__
  if (vm.contains("memlimit")) {
    rlimit rlim_memory{};
    getrlimit(RLIMIT_FSIZE, &rlim_memory);
    rlim_memory.rlim_cur = vm["memlimit"].as<rlim_t>();
    if (setrlimit(RLIMIT_AS, &rlim_memory) != 0) {
      spdlog::error("Failed to set memory limit {}", rlim_memory.rlim_cur);
      return 1;
    }
  }
#endif

  try {
    FileSignatureGenerator file_signature_generator;
    measure_time(
        &FileSignatureGenerator::run,
        &file_signature_generator,
        vm["in_file"].as<fs::path>(),
        vm["out_file"].as<fs::path>(),
        vm["block_size"].as<std::size_t>());
  } catch (std::exception const& ex) {
    spdlog::error(ex.what());
    return 1;
  }

  return 0;
}