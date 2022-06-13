#include "Signature.h"

#include <boost/program_options.hpp>

#include <filesystem>
#include <iostream>

namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, const char* argv[])
{
  po::options_description opt_desc{"Options"};

  //  constexpr std::string kInFile{"in_file"}; // TODO
  //  constexpr char* kOutFile{"out_file"};
  //  constexpr char* kBlockSize{"block_size"};

  // clang-format off
  opt_desc.add_options()
    ("help,h", "Display this information.")
    ("in_file,i", po::value<fs::path>()->required(), "Path to the input file")
    ("out_file,o", po::value<fs::path>()->required(), "Path to the output file")
    ("block_size,b", po::value<std::size_t>()->default_value(1000000), "Block size for hashing");
  // clang-format on

  po::variables_map vm;
  po::store(parse_command_line(argc, argv, opt_desc), vm);
  po::notify(vm);

  FileSignatureGenerator file_signature_generator;

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  file_signature_generator.run(
      vm["in_file"].as<fs::path>(),
      vm["out_file"].as<fs::path>(),
      vm["block_size"].as<std::size_t>());
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
            << "[ms]" << std::endl;
  std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]"
            << std::endl;
  std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::minutes>(end - begin).count() << "[min]"
            << std::endl;

  return 0;
}