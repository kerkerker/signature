#include "Signature.h"

#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

#include <concepts>
#include <filesystem>
#include <iostream>

#ifdef __linux__
#include <sys/resource.h>
#include <sys/time.h>
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

int main(int argc, const char* argv[])
{
#ifdef __linux__
  /* setting up soft limit on resources */
//s
#endif

  rlimit rlim_memory;
  getrlimit(RLIMIT_FSIZE, &rlim_memory);
  rlim_memory.rlim_cur = 500 * 1024 * 1024;
  if (setrlimit(RLIMIT_AS, &rlim_memory)) {
    return 2;
  }

  spdlog::set_level(spdlog::level::trace);

  po::options_description opt_desc{"Options"};

  //  constexpr std::string kInFile{"in_file"}; // TODO
  //  constexpr char* kOutFile{"out_file"};
  //  constexpr char* kBlockSize{"block_size"};

  auto block_size_check = [](std::size_t value) {
    //    std::cout << "in!" << std::endl;
    if (value <= 0) {
      throw po::validation_error(po::validation_error::invalid_option_value, "block_size", std::to_string(value));
    }
  };

  // clang-format off

  opt_desc.add_options()
    ("help,h", "Display this information.")
    ("in_file,i", po::value<fs::path>()->required(), "Path to the input file")
    ("out_file,o", po::value<fs::path>()->required(), "Path to the output file")
    ("block_size,b", po::value<std::size_t>()->default_value(1000000)->notifier(block_size_check
  ), "Block size for hashing (>0)");
  // clang-format on

  po::variables_map vm;
  try {
    po::store(parse_command_line(argc, argv, opt_desc), vm);
    po::notify(vm);
  } catch (boost::program_options::error const& ex) {
    spdlog::error(ex.what());
    return 0;
  }

  FileSignatureGenerator file_signature_generator;

  try {
    measure_time(
        &FileSignatureGenerator::run,
        &file_signature_generator,
        vm["in_file"].as<fs::path>(),
        vm["out_file"].as<fs::path>(),
        vm["block_size"].as<std::size_t>());
  } catch (std::exception const& ex) {
    spdlog::error(ex.what());
  }

  return 0;
}