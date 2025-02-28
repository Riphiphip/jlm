/*
 * Copyright 2022 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/opt/alias-analyses/Optimization.hpp>
#include <jlm/opt/cne.hpp>
#include <jlm/opt/DeadNodeElimination.hpp>
#include <jlm/opt/inlining.hpp>
#include <jlm/opt/InvariantValueRedirection.hpp>
#include <jlm/opt/pull.hpp>
#include <jlm/opt/push.hpp>
#include <jlm/opt/inversion.hpp>
#include <jlm/opt/unroll.hpp>
#include <jlm/opt/reduction.hpp>
#include <jlm/tooling/CommandLine.hpp>

#include <llvm/Support/CommandLine.h>

#include <unordered_map>

namespace jlm
{

CommandLineOptions::~CommandLineOptions()
= default;

std::string
JlcCommandLineOptions::ToString(const OptimizationLevel & optimizationLevel)
{
  static std::unordered_map<OptimizationLevel, const char*>
    map({
          {OptimizationLevel::O0, "O0"},
          {OptimizationLevel::O1, "O1"},
          {OptimizationLevel::O2, "O2"},
          {OptimizationLevel::O3, "O3"},
        });

  JLM_ASSERT(map.find(optimizationLevel) != map.end());
  return map[optimizationLevel];
}

std::string
JlcCommandLineOptions::ToString(const LanguageStandard & languageStandard)
{
  static std::unordered_map<LanguageStandard, const char*>
    map({
          {LanguageStandard::None, ""},
          {LanguageStandard::Gnu89, "gnu89"},
          {LanguageStandard::Gnu99, "gnu99"},
          {LanguageStandard::C89, "c89"},
          {LanguageStandard::C99, "c99"},
          {LanguageStandard::C11, "c11"},
          {LanguageStandard::Cpp98, "c++98"},
          {LanguageStandard::Cpp03, "c++03"},
          {LanguageStandard::Cpp11, "c++11"},
          {LanguageStandard::Cpp14, "c++14"}
        });

  JLM_ASSERT(map.find(languageStandard) != map.end());
  return map[languageStandard];
}

void
JlcCommandLineOptions::Reset() noexcept
{
  OnlyPrintCommands_ = false;
  GenerateDebugInformation_ = false;
  Verbose_ = false;
  Rdynamic_ = false;
  Suppress_ = false;
  UsePthreads_ = false;

  Md_ = false;

  OptimizationLevel_ = OptimizationLevel::O0;
  LanguageStandard_ = LanguageStandard::None;

  OutputFile_ = filepath("a.out");
  Libraries_.clear();
  MacroDefinitions_.clear();
  LibraryPaths_.clear();
  Warnings_.clear();
  IncludePaths_.clear();
  Flags_.clear();
  JlmOptOptimizations_.clear();

  Compilations_.clear();
}

void
JlmOptCommandLineOptions::Reset() noexcept
{
  InputFile_ = filepath("");
  OutputFile_ = filepath("");
  OutputFormat_ = OutputFormat::Llvm;
  StatisticsCollectorSettings_ = StatisticsCollectorSettings();
  Optimizations_.clear();
}

void
JlmHlsCommandLineOptions::Reset() noexcept
{
  InputFile_ = filepath("");
  OutputFolder_ = filepath("");
  OutputFormat_ = OutputFormat::Firrtl;
  HlsFunction_ = "";
  ExtractHlsFunction_ = false;
  UseCirct_ = false;
}

void
JhlsCommandLineOptions::Reset() noexcept
{
  *this = JhlsCommandLineOptions();
}

CommandLineParser::~CommandLineParser() noexcept
= default;

JlcCommandLineParser::~JlcCommandLineParser() noexcept
= default;

const JlcCommandLineOptions &
JlcCommandLineParser::ParseCommandLineArguments(int argc, char **argv)
{
  CommandLineOptions_.Reset();

  using namespace llvm;

  /*
    FIXME: The command line parser setup is currently redone
    for every invocation of parse_cmdline. We should be able
    to do it only once and then reset the parser on every
    invocation of parse_cmdline.
  */

  cl::TopLevelSubCommand->reset();

  cl::opt<bool> onlyPrintCommands(
    "###",
    cl::ValueDisallowed,
    cl::desc("Print (but do not run) the commands for this compilation."));

  cl::list<std::string> inputFiles(
    cl::Positional,
    cl::desc("<inputs>"));

  cl::list<std::string> includePaths(
    "I",
    cl::Prefix,
    cl::desc("Add directory <dir> to include search paths."),
    cl::value_desc("dir"));

  cl::list<std::string> libraryPaths(
    "L",
    cl::Prefix,
    cl::desc("Add directory <dir> to library search paths."),
    cl::value_desc("dir"));

  cl::list<std::string> libraries(
    "l",
    cl::Prefix,
    cl::desc("Search the library <lib> when linking."),
    cl::value_desc("lib"));

  cl::opt<std::string> outputFile(
    "o",
    cl::desc("Write output to <file>."),
    cl::value_desc("file"));

  cl::opt<bool> generateDebugInformation(
    "g",
    cl::ValueDisallowed,
    cl::desc("Generate source-level debug information."));

  cl::opt<bool> noLinking(
    "c",
    cl::ValueDisallowed,
    cl::desc("Only run preprocess, compile, and assemble steps."));

  cl::opt<std::string> optimizationLevel(
    "O",
    cl::Prefix,
    cl::ValueOptional,
    cl::desc("Optimization level. [O0, O1, O2, O3]"),
    cl::value_desc("#"));

  cl::list<std::string> macroDefinitions(
    "D",
    cl::Prefix,
    cl::desc("Add <macro> to preprocessor macros."),
    cl::value_desc("macro"));

  cl::list<std::string> warnings(
    "W",
    cl::Prefix,
    cl::desc("Enable specified warning."),
    cl::value_desc("warning"));

  cl::opt<std::string> languageStandard(
    "std",
    cl::desc("Language standard."),
    cl::value_desc("standard"));

  cl::list<std::string> flags(
    "f",
    cl::Prefix,
    cl::desc("Specify flags."),
    cl::value_desc("flag"));

  cl::list<std::string> jlmOptimizations(
    "J",
    cl::Prefix,
    cl::desc("jlm-opt optimization. Run 'jlm-opt -help' for viable options."),
    cl::value_desc("jlmopt"));

  cl::opt<bool> verbose(
    "v",
    cl::ValueDisallowed,
    cl::desc("Show commands to run and use verbose output. (Affects only clang for now)"));

  cl::opt<bool> rDynamic(
    "rdynamic",
    cl::ValueDisallowed,
    cl::desc("rDynamic option passed to clang"));

  cl::opt<bool> suppress(
    "w",
    cl::ValueDisallowed,
    cl::desc("Suppress all warnings"));

  cl::opt<bool> usePthreads(
    "pthread",
    cl::ValueDisallowed,
    cl::desc("Support POSIX threads in generated code"));

  cl::opt<bool> mD(
    "MD",
    cl::ValueDisallowed,
    cl::desc("Write a depfile containing user and system headers"));

  cl::opt<std::string> mF(
    "MF",
    cl::desc("Write depfile output from -mD to <file>."),
    cl::value_desc("file"));

  cl::opt<std::string> mT(
    "MT",
    cl::desc("Specify name of main file output in depfile."),
    cl::value_desc("value"));

  cl::ParseCommandLineOptions(argc, argv);

  /* Process parsed options */

  static std::unordered_map<std::string, jlm::JlcCommandLineOptions::OptimizationLevel> optimizationLevelMap(
    {
      {"0", JlcCommandLineOptions::OptimizationLevel::O0},
      {"1", JlcCommandLineOptions::OptimizationLevel::O1},
      {"2", JlcCommandLineOptions::OptimizationLevel::O2},
      {"3", JlcCommandLineOptions::OptimizationLevel::O3}
    });

  static std::unordered_map<std::string, jlm::JlcCommandLineOptions::LanguageStandard> languageStandardMap(
    {
      {"gnu89", jlm::JlcCommandLineOptions::LanguageStandard::Gnu89},
      {"gnu99", jlm::JlcCommandLineOptions::LanguageStandard::Gnu99},
      {"c89",   jlm::JlcCommandLineOptions::LanguageStandard::C89},
      {"c90",   jlm::JlcCommandLineOptions::LanguageStandard::C99},
      {"c99",   jlm::JlcCommandLineOptions::LanguageStandard::C99},
      {"c11",   jlm::JlcCommandLineOptions::LanguageStandard::C11},
      {"c++98", jlm::JlcCommandLineOptions::LanguageStandard::Cpp98},
      {"c++03", jlm::JlcCommandLineOptions::LanguageStandard::Cpp03},
      {"c++11", jlm::JlcCommandLineOptions::LanguageStandard::Cpp11},
      {"c++14", jlm::JlcCommandLineOptions::LanguageStandard::Cpp14}
    });

  if (!optimizationLevel.empty()) {
    auto iterator = optimizationLevelMap.find(optimizationLevel);
    if (iterator == optimizationLevelMap.end()) {
      std::cerr << "Unknown optimization level.\n";
      exit(EXIT_FAILURE);
    }
    CommandLineOptions_.OptimizationLevel_ = iterator->second;
  }

  if (!languageStandard.empty()) {
    auto iterator = languageStandardMap.find(languageStandard);
    if (iterator == languageStandardMap.end()) {
      std::cerr << "Unknown language standard.\n";
      exit(EXIT_FAILURE);
    }
    CommandLineOptions_.LanguageStandard_ = iterator->second;
  }

  if (inputFiles.empty()) {
    std::cerr << "jlc: no input files.\n";
    exit(EXIT_FAILURE);
  }

  if (inputFiles.size() > 1 && noLinking && !outputFile.empty()) {
    std::cerr << "jlc: cannot specify -o when generating multiple output files.\n";
    exit(EXIT_FAILURE);
  }

  CommandLineOptions_.Libraries_ = libraries;
  CommandLineOptions_.MacroDefinitions_ = macroDefinitions;
  CommandLineOptions_.LibraryPaths_ = libraryPaths;
  CommandLineOptions_.Warnings_ = warnings;
  CommandLineOptions_.IncludePaths_ = includePaths;
  CommandLineOptions_.OnlyPrintCommands_ = onlyPrintCommands;
  CommandLineOptions_.GenerateDebugInformation_ = generateDebugInformation;
  CommandLineOptions_.Flags_ = flags;
  CommandLineOptions_.JlmOptOptimizations_ = jlmOptimizations;
  CommandLineOptions_.Verbose_ = verbose;
  CommandLineOptions_.Rdynamic_ = rDynamic;
  CommandLineOptions_.Suppress_ = suppress;
  CommandLineOptions_.UsePthreads_ = usePthreads;
  CommandLineOptions_.Md_ = mD;

  for (auto & inputFile : inputFiles) {
    if (IsObjectFile(inputFile)) {
      /* FIXME: print a warning like clang if noLinking is true */
      CommandLineOptions_.Compilations_.push_back({
                                                    inputFile,
                                                    jlm::filepath(""),
                                                    inputFile,
                                                    "",
                                                    false,
                                                    false,
                                                    false,
                                                    true});

      continue;
    }

    CommandLineOptions_.Compilations_.push_back({
                                                  inputFile,
                                                  mF.empty() ? ToDependencyFile(inputFile) : jlm::filepath(mF),
                                                  ToObjectFile(inputFile),
                                                  mT.empty() ? ToObjectFile(inputFile).name() : mT,
                                                  true,
                                                  true,
                                                  true,
                                                  !noLinking});
  }

  if (!outputFile.empty()) {
    if (noLinking) {
      JLM_ASSERT(CommandLineOptions_.Compilations_.size() == 1);
      CommandLineOptions_.Compilations_[0].SetOutputFile(outputFile);
    } else {
      CommandLineOptions_.OutputFile_ = filepath(outputFile);
    }
  }

  return CommandLineOptions_;
}

JlmOptCommandLineParser::~JlmOptCommandLineParser() noexcept
= default;

optimization *
JlmOptCommandLineParser::GetOptimization(enum OptimizationId id)
{
  static aa::SteensgaardAgnostic steensgaardAgnostic;
  static aa::SteensgaardRegionAware steensgaardRegionAware;
  static cne commonNodeElimination;
  static DeadNodeElimination deadNodeElimination;
  static fctinline functionInlining;
  static InvariantValueRedirection invariantValueRedirection;
  static pullin nodePullIn;
  static pushout nodePushOt;
  static tginversion thetaGammaInversion;
  static loopunroll loopUnrolling(4);
  static nodereduction nodeReduction;

  static std::unordered_map<OptimizationId, jlm::optimization*> map(
    {
      {OptimizationId::AASteensgaardAgnostic,     &steensgaardAgnostic},
      {OptimizationId::AASteensgaardRegionAware,  &steensgaardRegionAware},
      {OptimizationId::cne,                       &commonNodeElimination},
      {OptimizationId::dne,                       &deadNodeElimination},
      {OptimizationId::iln,                       &functionInlining},
      {OptimizationId::InvariantValueRedirection, &invariantValueRedirection},
      {OptimizationId::pll,                       &nodePullIn},
      {OptimizationId::psh,                       &nodePushOt},
      {OptimizationId::ivt,                       &thetaGammaInversion},
      {OptimizationId::url,                       &loopUnrolling},
      {OptimizationId::red,                       &nodeReduction}
    });

  JLM_ASSERT(map.find(id) != map.end());
  return map[id];
}

const JlmOptCommandLineOptions &
JlmOptCommandLineParser::ParseCommandLineArguments(int argc, char **argv)
{
  CommandLineOptions_.Reset();

  using namespace llvm;

  /*
    FIXME: The command line parser setup is currently redone
    for every invocation of parse_cmdline. We should be able
    to do it only once and then reset the parser on every
    invocation of parse_cmdline.
  */

  cl::TopLevelSubCommand->reset();

  cl::opt<std::string> inputFile(
    cl::Positional,
    cl::desc("<input>"));

  cl::opt<std::string> outputFile(
    "o",
    cl::desc("Write output to <file>"),
    cl::value_desc("file"));

  cl::opt<std::string> statisticFile(
    "s",
    cl::desc("Write stats to <file>. Default is "
             + CommandLineOptions_.StatisticsCollectorSettings_.GetFilePath().to_str()
             + "."),
    cl::value_desc("file"));

  cl::list<Statistics::Id> printStatistics(
    cl::values(
      clEnumValN(
        Statistics::Id::Aggregation,
        "print-aggregation-time",
        "Write aggregation statistics to file."),
      clEnumValN(
        Statistics::Id::Annotation,
        "print-annotation-time",
        "Write annotation statistics to file."),
      clEnumValN(
        Statistics::Id::BasicEncoderEncoding,
        "print-basicencoder-encoding",
        "Write encoding statistics of basic encoder to file."),
      clEnumValN(
        Statistics::Id::CommonNodeElimination,
        "print-cne-stat",
        "Write common node elimination statistics to file."),
      clEnumValN(
        Statistics::Id::ControlFlowRecovery,
        "print-cfr-time",
        "Write control flow recovery statistics to file."),
      clEnumValN(
        Statistics::Id::DataNodeToDelta,
        "printDataNodeToDelta",
        "Write data node to delta node conversion statistics to file."),
      clEnumValN(
        Statistics::Id::DeadNodeElimination,
        "print-dne-stat",
        "Write dead node elimination statistics to file."),
      clEnumValN(
        Statistics::Id::FunctionInlining,
        "print-iln-stat",
        "Write function inlining statistics to file."),
      clEnumValN(
        Statistics::Id::InvariantValueRedirection,
        "printInvariantValueRedirection",
        "Write invariant value redirection statistics to file."),
      clEnumValN(
        Statistics::Id::JlmToRvsdgConversion,
        "print-jlm-rvsdg-conversion",
        "Write Jlm to RVSDG conversion statistics to file."),
      clEnumValN(
        Statistics::Id::LoopUnrolling,
        "print-unroll-stat",
        "Write loop unrolling statistics to file."),
      clEnumValN(
        Statistics::Id::MemoryNodeProvisioning,
        "print-memory-node-provisioning",
        "Write memory node provisioning statistics to file."),
      clEnumValN(
        Statistics::Id::PullNodes,
        "print-pull-stat",
        "Write node pull statistics to file."),
      clEnumValN(
        Statistics::Id::PushNodes,
        "print-push-stat",
        "Write node push statistics to file."),
      clEnumValN(
        Statistics::Id::ReduceNodes,
        "print-reduction-stat",
        "Write node reduction statistics to file."),
      clEnumValN(
        Statistics::Id::RvsdgConstruction,
        "print-rvsdg-construction",
        "Write RVSDG construction statistics to file."),
      clEnumValN(
        Statistics::Id::RvsdgDestruction,
        "print-rvsdg-destruction",
        "Write RVSDG destruction statistics to file."),
      clEnumValN(
        Statistics::Id::RvsdgOptimization,
        "print-rvsdg-optimization",
        "Write RVSDG optimization statistics to file."),
      clEnumValN(
        Statistics::Id::SteensgaardAnalysis,
        "print-steensgaard-analysis",
        "Write Steensgaard analysis statistics to file."),
      clEnumValN(
        Statistics::Id::SteensgaardPointsToGraphConstruction,
        "print-steensgaard-pointstograph-construction",
        "Write Steensgaard PointsTo Graph construction statistics to file."),
      clEnumValN(
        Statistics::Id::ThetaGammaInversion,
        "print-ivt-stat",
        "Write theta-gamma inversion statistics to file.")),
    cl::desc("Write statistics"));

  cl::opt<JlmOptCommandLineOptions::OutputFormat> outputFormat(
    cl::values(
      clEnumValN(
        JlmOptCommandLineOptions::OutputFormat::Llvm,
        "llvm",
        "Output LLVM IR [default]"),
      clEnumValN(
        JlmOptCommandLineOptions::OutputFormat::Xml,
        "xml",
        "Output XML")),
    cl::desc("Select output format"));

  cl::list<OptimizationId> optimizationIds(
    cl::values(
      clEnumValN(
        OptimizationId::AASteensgaardAgnostic,
        "AASteensgaardAgnostic",
        "Steensgaard alias analysis with agnostic memory state encoding."),
      clEnumValN(
        OptimizationId::AASteensgaardRegionAware,
        "AASteensgaardRegionAware",
        "Steensgaard alias analysis with region-aware memory state encoding."),
      clEnumValN(
        OptimizationId::cne,
        "cne",
        "Common node elimination"),
      clEnumValN(
        OptimizationId::dne,
        "dne",
        "Dead node elimination"),
      clEnumValN(
        OptimizationId::iln,
        "iln",
        "Function inlining"),
      clEnumValN(
        OptimizationId::InvariantValueRedirection,
        "InvariantValueRedirection",
        "Invariant Value Redirection"),
      clEnumValN(
        OptimizationId::psh,
        "psh",
        "Node push out"),
      clEnumValN(
        OptimizationId::pll,
        "pll",
        "Node pull in"),
      clEnumValN(
        OptimizationId::red,
        "red",
        "Node reductions"),
      clEnumValN(
        OptimizationId::ivt,
        "ivt",
        "Theta-gamma inversion"),
      clEnumValN(
        OptimizationId::url,
        "url",
        "Loop unrolling")),
    cl::desc("Perform optimization"));

  cl::ParseCommandLineOptions(argc, argv);

  if (!outputFile.empty())
    CommandLineOptions_.OutputFile_ = outputFile;

  if (!statisticFile.empty())
    CommandLineOptions_.StatisticsCollectorSettings_.SetFilePath(statisticFile);

  std::vector<jlm::optimization*> optimizations;
  for (auto & optimizationId : optimizationIds)
    optimizations.push_back(GetOptimization(optimizationId));

  HashSet<Statistics::Id> printStatisticsIds({printStatistics.begin(), printStatistics.end()});

  CommandLineOptions_.InputFile_ = inputFile;
  CommandLineOptions_.OutputFormat_ = outputFormat;
  CommandLineOptions_.Optimizations_ = optimizations;
  CommandLineOptions_.StatisticsCollectorSettings_.SetDemandedStatistics(printStatisticsIds);

  return CommandLineOptions_;
}

const JlmOptCommandLineOptions &
JlmOptCommandLineParser::Parse(int argc, char ** argv)
{
  static JlmOptCommandLineParser parser;
  return parser.ParseCommandLineArguments(argc, argv);
}

JlmHlsCommandLineParser::~JlmHlsCommandLineParser() noexcept
= default;

const JlmHlsCommandLineOptions &
JlmHlsCommandLineParser::ParseCommandLineArguments(int argc, char ** argv)
{
  CommandLineOptions_.Reset();

  using namespace llvm;

  /*
    FIXME: The command line parser setup is currently redone
    for every invocation of parse_cmdline. We should be able
    to do it only once and then reset the parser on every
    invocation of parse_cmdline.
  */

  cl::TopLevelSubCommand->reset();

  cl::opt<std::string> inputFile(
    cl::Positional,
    cl::desc("<input>"));

  cl::opt<std::string> outputFolder(
    "o",
    cl::desc("Write output to <folder>"),
    cl::value_desc("folder"));

  cl::opt<std::string> hlsFunction(
    "hls-function",
    cl::Prefix,
    cl::desc("Function that should be accelerated"),
    cl::value_desc("hls-function"));

  cl::opt<bool> extractHlsFunction(
    "extract",
    cl::Prefix,
    cl::desc("Extracts function specified by hls-function"));

  cl::opt<bool> useCirct(
    "circt",
    cl::Prefix,
    cl::desc("Use CIRCT to generate FIRRTL"));

  cl::opt<JlmHlsCommandLineOptions::OutputFormat> format(
    cl::values(
      clEnumValN(
        JlmHlsCommandLineOptions::OutputFormat::Firrtl,
        "fir",
        "Output FIRRTL [default]"),
      clEnumValN(
        JlmHlsCommandLineOptions::OutputFormat::Dot,
        "dot",
        "Output DOT graph")),
    cl::desc("Select output format"));

  cl::ParseCommandLineOptions(argc, argv);

  if (outputFolder.empty())
    throw jlm::error("jlm-hls no output directory provided, i.e, -o.\n");

  if (extractHlsFunction && hlsFunction.empty())
    throw jlm::error("jlm-hls: --hls-function is not specified.\n         which is required for --extract\n");

  CommandLineOptions_.InputFile_ = inputFile;
  CommandLineOptions_.HlsFunction_ = std::move(hlsFunction);
  CommandLineOptions_.OutputFolder_ = outputFolder;
  CommandLineOptions_.ExtractHlsFunction_ = extractHlsFunction;
  CommandLineOptions_.UseCirct_ = useCirct;
  CommandLineOptions_.OutputFormat_ = format;

  return CommandLineOptions_;
}

const JlmHlsCommandLineOptions &
JlmHlsCommandLineParser::Parse(int argc, char ** argv)
{
  static JlmHlsCommandLineParser parser;
  return parser.ParseCommandLineArguments(argc, argv);
}

JhlsCommandLineParser::~JhlsCommandLineParser() noexcept
= default;

const JhlsCommandLineOptions &
JhlsCommandLineParser::ParseCommandLineArguments(int argc, char **argv)
{
  CommandLineOptions_.Reset();

  using namespace llvm;

  /*
    FIXME: The command line parser setup is currently redone
    for every invocation of parse_cmdline. We should be able
    to do it only once and then reset the parser on every
    invocation of parse_cmdline.
  */

  cl::TopLevelSubCommand->reset();

  cl::opt<bool> onlyPrintCommands(
    "###",
    cl::ValueDisallowed,
    cl::desc("Print (but do not run) the commands for this compilation."));

  cl::list<std::string> inputFiles(
    cl::Positional,
    cl::desc("<inputs>"));

  cl::list<std::string> includePaths(
    "I",
    cl::Prefix,
    cl::desc("Add directory <dir> to include search paths."),
    cl::value_desc("dir"));

  cl::list<std::string> libraryPaths(
    "L",
    cl::Prefix,
    cl::desc("Add directory <dir> to library search paths."),
    cl::value_desc("dir"));

  cl::list<std::string> libraries(
    "l",
    cl::Prefix,
    cl::desc("Search the library <lib> when linking."),
    cl::value_desc("lib"));

  cl::opt<std::string> outputFile(
    "o",
    cl::desc("Write output to <file>."),
    cl::value_desc("file"));

  cl::opt<bool> generateDebugInformation(
    "g",
    cl::ValueDisallowed,
    cl::desc("Generate source-level debug information."));

  cl::opt<bool> noLinking(
    "c",
    cl::ValueDisallowed,
    cl::desc("Only run preprocess, compile, and assemble steps."));

  cl::opt<std::string> optimizationLevel(
    "O",
    cl::Prefix,
    cl::ValueOptional,
    cl::desc("Optimization level. [O0, O1, O2, O3]"),
    cl::value_desc("#"));

  cl::list<std::string> macroDefinitions(
    "D",
    cl::Prefix,
    cl::desc("Add <macro> to preprocessor macros."),
    cl::value_desc("macro"));

  cl::list<std::string> warnings(
    "W",
    cl::Prefix,
    cl::desc("Enable specified warning."),
    cl::value_desc("warning"));

  cl::opt<std::string> languageStandard(
    "std",
    cl::desc("Language standard."),
    cl::value_desc("standard"));

  cl::list<std::string> flags(
    "f",
    cl::Prefix,
    cl::desc("Specify flags."),
    cl::value_desc("flag"));

  cl::list<std::string> jlmHlsOptimizations(
    "J",
    cl::Prefix,
    cl::desc("jlm-hls optimization. Run 'jlm-hls -help' for viable options."),
    cl::value_desc("jlmhls"));

  cl::opt<bool> verbose(
    "v",
    cl::ValueDisallowed,
    cl::desc("Show commands to run and use verbose output. (Affects only clang for now)"));

  cl::opt<bool> rdynamic(
    "rdynamic",
    cl::ValueDisallowed,
    cl::desc("rdynamic option passed to clang"));

  cl::opt<bool> suppress(
    "w",
    cl::ValueDisallowed,
    cl::desc("Suppress all warnings"));

  cl::opt<bool> usePthreads(
    "pthread",
    cl::ValueDisallowed,
    cl::desc("Support POSIX threads in generated code"));

  cl::opt<bool> mD(
    "MD",
    cl::ValueDisallowed,
    cl::desc("Write a depfile containing user and system headers"));

  cl::opt<std::string> mF(
    "MF",
    cl::desc("Write depfile output from -MD to <file>."),
    cl::value_desc("file"));

  cl::opt<std::string> mT(
    "MT",
    cl::desc("Specify name of main file output in depfile."),
    cl::value_desc("value"));

  cl::list<std::string> hlsFunction(
    "hls-function",
    cl::Prefix,
    cl::desc("function that should be accelerated"),
    cl::value_desc("regex"));

  cl::opt<bool> generateFirrtl(
    "firrtl",
    cl::ValueDisallowed,
    cl::desc("Generate firrtl"));

  cl::opt<bool> useCirct(
    "circt",
    cl::Prefix,
    cl::desc("Use CIRCT to generate FIRRTL"));

  cl::ParseCommandLineOptions(argc, argv);

  /* Process parsed options */

  static std::unordered_map<std::string, JhlsCommandLineOptions::OptimizationLevel> Olvlmap(
    {
      {"0", JhlsCommandLineOptions::OptimizationLevel::O0},
      {"1", JhlsCommandLineOptions::OptimizationLevel::O1},
      {"2", JhlsCommandLineOptions::OptimizationLevel::O2},
      {"3", JhlsCommandLineOptions::OptimizationLevel::O3}
    });

  static std::unordered_map<std::string, JhlsCommandLineOptions::LanguageStandard> stdmap(
    {
      {"Gnu89", JhlsCommandLineOptions::LanguageStandard::Gnu89},
      {"Gnu99", JhlsCommandLineOptions::LanguageStandard::Gnu99},
      {"C89", JhlsCommandLineOptions::LanguageStandard::C89},
      {"C99", JhlsCommandLineOptions::LanguageStandard::C99},
      {"C11", JhlsCommandLineOptions::LanguageStandard::C11},
      {"C++98", JhlsCommandLineOptions::LanguageStandard::Cpp98},
      {"C++03", JhlsCommandLineOptions::LanguageStandard::Cpp03},
      {"C++11", JhlsCommandLineOptions::LanguageStandard::Cpp11},
      {"C++14", JhlsCommandLineOptions::LanguageStandard::Cpp14}
    });

  if (!optimizationLevel.empty()) {
    auto it = Olvlmap.find(optimizationLevel);
    if (it == Olvlmap.end()) {
      std::cerr << "Unknown optimization level.\n";
      exit(EXIT_FAILURE);
    }
    CommandLineOptions_.OptimizationLevel_ = it->second;
  }

  if (!languageStandard.empty()) {
    auto it = stdmap.find(languageStandard);
    if (it == stdmap.end()) {
      std::cerr << "Unknown language standard.\n";
      exit(EXIT_FAILURE);
    }
    CommandLineOptions_.LanguageStandard_ = it->second;
  }

  if (inputFiles.empty()) {
    std::cerr << "jlc: no input files.\n";
    exit(EXIT_FAILURE);
  }

  if (inputFiles.size() > 1 && noLinking && !outputFile.empty()) {
    std::cerr << "jlc: cannot specify -o when generating multiple output files.\n";
    exit(EXIT_FAILURE);
  }

  if (!hlsFunction.empty()) {
    CommandLineOptions_.Hls_ = true;
    CommandLineOptions_.HlsFunctionRegex_ = hlsFunction.front();
  }

  if (hlsFunction.size() > 1) {
    std::cerr << "jlc-hls: more than one function regex specified\n";
    exit(EXIT_FAILURE);
  }

  CommandLineOptions_.Libraries_ = libraries;
  CommandLineOptions_.MacroDefinitions_ = macroDefinitions;
  CommandLineOptions_.LibraryPaths_ = libraryPaths;
  CommandLineOptions_.Warnings_ = warnings;
  CommandLineOptions_.IncludePaths_ = includePaths;
  CommandLineOptions_.OnlyPrintCommands_ = onlyPrintCommands;
  CommandLineOptions_.GenerateDebugInformation_ = generateDebugInformation;
  CommandLineOptions_.Flags_ = flags;
  CommandLineOptions_.JlmHls_ = jlmHlsOptimizations;
  CommandLineOptions_.Verbose_ = verbose;
  CommandLineOptions_.Rdynamic_ = rdynamic;
  CommandLineOptions_.Suppress_ = suppress;
  CommandLineOptions_.UsePthreads_ = usePthreads;
  CommandLineOptions_.Md_ = mD;
  CommandLineOptions_.GenerateFirrtl_ = generateFirrtl;
  CommandLineOptions_.UseCirct_ = useCirct;

  for (auto & inputFile : inputFiles) {
    if (IsObjectFile(inputFile)) {
      /* FIXME: print a warning like clang if noLinking is true */
      CommandLineOptions_.Compilations_.push_back({
                                                    inputFile,
                                                    filepath(""),
                                                    inputFile,
                                                    "",
                                                    false,
                                                    false,
                                                    false,
                                                    true});

      continue;
    }

    CommandLineOptions_.Compilations_.push_back({
                                                  inputFile,
                                                  mF.empty() ? CreateDependencyFileFromFile(inputFile) : filepath(mF),
                                                  CreateObjectFileFromFile(inputFile),
                                                  mT.empty() ? CreateObjectFileFromFile(inputFile).name() : mT,
                                                  true,
                                                  true,
                                                  true,
                                                  !noLinking});
  }

  if (!outputFile.empty()) {
    if (noLinking) {
      JLM_ASSERT(CommandLineOptions_.Compilations_.size() == 1);
      CommandLineOptions_.Compilations_[0].SetOutputFile(outputFile);
    } else {
      CommandLineOptions_.OutputFile_ = jlm::filepath(outputFile);
    }
  }

  return CommandLineOptions_;
}

bool
JhlsCommandLineParser::IsObjectFile(const filepath & file)
{
  return file.suffix() == "o";
}

filepath
JhlsCommandLineParser::CreateObjectFileFromFile(const filepath & f)
{
  return {f.path() + f.base() + ".o"};
}

filepath
JhlsCommandLineParser::CreateDependencyFileFromFile(const filepath & f)
{
  return {f.path() + f.base() + ".d"};
}

const JhlsCommandLineOptions &
JhlsCommandLineParser::Parse(int argc, char ** argv)
{
  static JhlsCommandLineParser parser;
  return parser.ParseCommandLineArguments(argc, argv);
}

}