/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/view.h>

#include <jlm/ir/module.hpp>
#include <jlm/ir/rvsdg.hpp>
#include <jlm/jlm2rvsdg/module.hpp>
#include <jlm/jlm2llvm/jlm2llvm.hpp>
#include <jlm/llvm2jlm/module.hpp>
#include <jlm/opt/dne.hpp>
#include <jlm/opt/inlining.hpp>
#include <jlm/opt/invariance.hpp>
#include <jlm/rvsdg2jlm/rvsdg2jlm.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include <iostream>

enum class opt {dne, iln, inv};

struct cmdflags {
	inline
	cmdflags()
	: xml(false)
	, llvm(false)
	{}

	bool xml;
	bool llvm;
	std::vector<opt> passes;
};

static void
print_usage(const std::string & app)
{
	std::cerr << "Usage: " << app << " [OPTIONS] FILE\n";
	std::cerr << "OPTIONS:\n";
	std::cerr << "--dne: Perform dead node elimination.\n";
	std::cerr << "--iln: Perform function inlining.\n";
	std::cerr << "--inv: Perform invariant value redirection.\n";
	std::cerr << "--llvm: Output LLVM IR.\n";
	std::cerr << "--xml: Output RVSDG as XML.\n";
}

static std::string
parse_cmdflags(int argc, char ** argv, cmdflags & flags)
{
	if (argc < 2) {
		std::cerr << "Expected LLVM IR file as input.\n";
		print_usage(argv[0]);
		exit(1);
	}

	static std::unordered_map<std::string, void(*)(cmdflags&)> map({
	  {"--dne", [](cmdflags & flags){ flags.passes.push_back(opt::dne); }}
	, {"--iln", [](cmdflags & flags){ flags.passes.push_back(opt::iln); }}
	, {"--inv", [](cmdflags & flags){ flags.passes.push_back(opt::inv); }}
	, {"--llvm", [](cmdflags & flags){ flags.llvm = true; }}
	, {"--xml", [](cmdflags & flags){ flags.xml = true; }}
	});

	for (int n = 1; n < argc-1; n++) {
		std::string flag(argv[n]);
		if (map.find(flag) != map.end()) {
			map[flag](flags);
			continue;
		}

		std::cerr << "Unknown command line flag: " << flag << "\n";
		print_usage(argv[0]);
		exit(1);
	}

	return std::string(argv[argc-1]);
}

static void
perform_optimizations(jive::graph * graph, const std::vector<opt> & opts)
{
	for (const auto & opt : opts) {
		if (opt == opt::dne) {
			jlm::dne(*graph);
			continue;
		}

		if (opt == opt::iln) {
			jlm::inlining(*graph);
			continue;
		}

		if (opt == opt::inv) {
			jlm::invariance(*graph);
			continue;
		}
	}
}

int
main(int argc, char ** argv)
{
	cmdflags flags;
	auto file = parse_cmdflags(argc, argv, flags);

	llvm::SMDiagnostic d;
	auto lm = llvm::parseIRFile(file, d, llvm::getGlobalContext());
	if (!lm) {
		d.print(argv[0], llvm::errs());
		exit(1);
	}

	auto jm = jlm::convert_module(*lm);
	auto rvsdg = jlm::construct_rvsdg(*jm);

	perform_optimizations(rvsdg->graph(), flags.passes);

	if (flags.llvm) {
		jm = jlm::rvsdg2jlm::rvsdg2jlm(*rvsdg);
		lm = jlm::jlm2llvm::convert(*jm, llvm::getGlobalContext());

		llvm::raw_os_ostream os(std::cout);
		lm->print(os, nullptr);
	}

	if (flags.xml)
		jive::view_xml(rvsdg->graph()->root(), stdout);

	return 0;
}
