/*
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/backend/llvm/jlm2llvm/jlm2llvm.hpp>
#include <jlm/backend/llvm/rvsdg2jlm/rvsdg2jlm.hpp>
#include <jlm/frontend/llvm/InterProceduralGraphConversion.hpp>
#include <jlm/frontend/llvm/LlvmModuleConversion.hpp>
#include <jlm/ir/ipgraph-module.hpp>
#include <jlm/ir/print.hpp>
#include <jlm/ir/RvsdgModule.hpp>
#include <jlm/util/Statistics.hpp>

#include <jive/view.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include <iostream>

#include <getopt.h>
#include "jlm/ir/operators/lambda.hpp"
#include "jlm/ir/operators/gamma.hpp"
#include "jlm/ir/operators/theta.hpp"
#include "jive/types/bitstring/type.hpp"
#include "jive/rvsdg/traverser.hpp"
#include "jive/types/bitstring/arithmetic.hpp"
#include "jive/types/bitstring/constant.hpp"
#include "jlm/opt/DeadNodeElimination.hpp"

class cmdflags {
public:
	inline
	cmdflags()
	{}

	std::string file;
};

static void
print_usage(const std::string & app)
{
	std::cerr << "Usage: " << app << " [OPTIONS]\n";
	std::cerr << "OPTIONS:\n";
	std::cerr << "--file name: LLVM IR file.\n";
}

static void
parse_cmdflags(int argc, char ** argv, cmdflags & flags)
{
	static constexpr size_t file = 0;

	static struct option options[] = {
	  {"file", required_argument, NULL, file}
	, {NULL, 0, NULL, 0}
	};

	int opt;
	while ((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case file: { flags.file = optarg; break; }

			default:
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (flags.file.empty()) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
}
class PrintMLIR {
    std::unordered_map<const jive::output *, std::string> output_map;
    int output_ctr = 0;

    static inline std::string
    indent(size_t depth) {
        return std::string(depth * 4, ' ');
    }

    std::string print_type(const jive::type *t){
        std::ostringstream s;
        if(auto bt = dynamic_cast<const jive::bittype *>(t)){
            s << "i" << bt->nbits();
        } else {
            return t->debug_string();
//            throw jlm::error("Printing for type " + t->debug_string() + " not implemented!");
        }
        return s.str();
    }

    std::string print_output(const jive::output *output){
        // assign a unique identifier to each output
        if(!output_map.count(output)){
            std::ostringstream s;
            s << "%" << output_ctr++;
            output_map[output] = s.str();
        }
        return output_map[output];
    }

    inline std::string print_input_origin(const jive::input *input) {
        return print_output(input->origin());
    }

    std::string print_simple_node(jive::simple_node *node, int indent_lvl) {
        std::ostringstream s;
        if (auto o = dynamic_cast<const jive::bitconstant_op *>(&(node->operation()))) {
            auto value = o->value();
            s << "arith.constant ";
            s << value.to_int();
            s << ": " << print_type(&node->output(0)->type());
        } else {
            // TODO: lookup op name
            s << node->operation().debug_string() << " (";
            for (size_t i = 0; i < node->ninputs(); ++i) {
                if(i!=0){
                    s << ", ";
                }
                s << print_input_origin(node->input(i)) << ": " << print_type(&node->input(i)->type());
            }
            s << ")";
            if(node->noutputs()){
                s << ": ";
                for (size_t i = 0; i < node->noutputs(); ++i) {
                    if(i!=0){
                        s << ", ";
                    }
                    s << print_type(&node->output(i)->type());
                }
            }
        }
        s << "\n";
        return s.str();
    }

    std::string print_subregion(jive::region *region, int indent_lvl, std::string result_type, int split_result_operands_at = -1) {
        std::ostringstream s;
        // arguments
        s << indent(indent_lvl) << "(";
        for (size_t i = 0; i < region->narguments(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_output(region->argument(i)) << ": " << print_type(&region->argument(i)->type());
        }
        s << "): {\n";
        // nodes/operations
        for (auto &node: jive::topdown_traverser(region)) {
            s << indent(indent_lvl+1);
            for (size_t i = 0; i < node->noutputs(); ++i) {
                if(i!=0){
                    s << ", ";
                }
                s << print_output(node->output(i));
            }
            if(node->noutputs()){
                s << " = ";
            }
            if (auto sn = dynamic_cast<jive::simple_node *>(node)) {
                s << print_simple_node(sn, indent_lvl+1);
            } else if(auto gamma = dynamic_cast<jive::gamma_node *>(node)) {
                s << print_gamma(gamma, indent_lvl+1);
            }  else if(auto theta = dynamic_cast<jive::theta_node *>(node)) {
                // TODO: handle gamma similar to lambda
            } else {
                throw jlm::error("Structural node"+node->operation().debug_string()+"not implemented yet");
            }
        }
        // results
        if(region->nresults()){
            s << indent(indent_lvl+1) << result_type << "(";
            for (size_t i = 0; i < region->nresults(); ++i) {
                if(i!=0 && !(split_result_operands_at > 0 && i==(size_t)split_result_operands_at)){
                    s << ", ";
                }
                if (split_result_operands_at >= 0 && i == (size_t)split_result_operands_at) {
                    s << "): (";
                }
                s << print_input_origin(region->result(i)) << ":" << print_type(&region->result(i)->type());
            }
            s << ")\n";
        }
        s << indent(indent_lvl) << "}";
        return s.str();
    }

    std::string print_lambda(const jlm::lambda::node &ln, int indent_lvl = 0) {
        std::ostringstream s;
        s << "rvsdg.lambdaNode <(";
        // fctarguments
        for (size_t i = 0; i < ln.nfctarguments(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_type(&ln.fctargument(i)->type());
        }
        s << ")->(";
        // fctresults
        for (size_t i = 0; i < ln.nfctresults(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_type(&ln.fctresult(i)->type());
        }
        s << ")> (";
        // context arguments
        for (size_t i = 0; i < ln.ncvarguments(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_input_origin(ln.cvargument(i)->input()) << ": " << print_type(&ln.cvargument(i)->type());
        }
        s << "):\n";
        s << print_subregion(ln.subregion(), indent_lvl+1, "rvsdg.lambdaResult");
        s << "\n";

        return s.str();
    }

    std::string print_gamma(const jive::gamma_node *gn, int indent_lvl = 0) {
        std::ostringstream s;
        s << "rvsdg.gammaNode";
        s << "(" << print_input_origin(gn->predicate()) << "): ";
        s << "(";
        for (size_t i = 1; i < gn->ninputs(); ++i) { // Predicate is input 0. Skip it here
            if(i!=0 && i!=1){
                s << ", ";
            }
            s << print_input_origin(gn->input(i)) << ": " << print_type(&gn->input(i)->type());
        }
        s << "): [\n";
        for (size_t i=0; i < gn->nsubregions(); ++i) {
            if (i != 0) {
                s << ",\n";
            }
            s << print_subregion(gn->subregion(i), indent_lvl+1, "rvsdg.gammaResult");
        }
        s << "\n";
        s << indent(indent_lvl) << "]->";
        for (size_t i = 0; i < gn->noutputs(); ++i) {
            if (i != 0) {
                s << ", ";
            }
            s << print_type(&gn->output(i)->type());
        }
        s << "\n";
        return s.str();
    }
public:
    std::string print_mlir(jlm::RvsdgModule &rm) {
        auto &graph = rm.Rvsdg();
        auto root = graph.root();
        std::ostringstream s;
        // if (root->nodes.size() != 1) {
        //     throw jlm::error("Root should have only one node for now");
        // }
        for (auto &node: root->nodes){
            auto ln = dynamic_cast<const jlm::lambda::node *>(&node);
            if (ln) {
                s << print_lambda(*ln);
        }
        }
        return s.str();
    }
};

int
main (int argc, char ** argv)
{
	cmdflags flags;
	parse_cmdflags(argc, argv, flags);

	llvm::LLVMContext ctx;
	llvm::SMDiagnostic err;
	auto lm = llvm::parseIRFile(flags.file, err, ctx);

	if (!lm) {
		err.print(argv[0], llvm::errs());
		exit(1);
	}

  jlm::StatisticsCollector statisticsCollector;

	/* LLVM to JLM pass */
	auto jm = jlm::ConvertLlvmModule(*lm);

	auto rvsdgModule = jlm::ConvertInterProceduralGraphModule(*jm, statisticsCollector);
    jlm::DeadNodeElimination dne;
    // run dead node elimination to skip some garbage
    dne.run(*rvsdgModule, statisticsCollector);
    jive::view(rvsdgModule->Rvsdg().root(), stderr);
    PrintMLIR printMlir;
    std::cout << printMlir.print_mlir(*rvsdgModule);

	return 0;
}
