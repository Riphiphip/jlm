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
#include <jive/rvsdg/control.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include <iostream>
#include <algorithm>

#include <getopt.h>
#include "jlm/ir/operators/lambda.hpp"
#include "jlm/ir/operators/gamma.hpp"
#include "jlm/ir/operators/theta.hpp"
#include "jlm/ir/operators/delta.hpp"
#include "jlm/ir/operators.hpp"
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
    std::unordered_map<const jive::output *, const jive::bitconstant_op *> constant_op_map;
    // Used as stack for tracking struct names. Needed to prevent infinite recursion.
    std::vector<std::string> struct_stack;
    int output_ctr = 0;

    static inline std::string
    indent(size_t depth) {
        return std::string(depth * 4, ' ');
    }

    std::string print_function_type(const jlm::FunctionType *ft) {
        std::ostringstream s;
        s << "!rvsdg.lambdaRef<(";
        for (size_t i = 0; i < ft->NumArguments(); ++i) {
            if (i!=0) {
                s << ", ";
            }
            s << print_type(&ft->ArgumentType(i));
        }
        s << ") -> (";
        for (size_t i = 0; i < ft->NumResults(); ++i) {
            if (i!=0) {
                s << ", ";
            }
            s << print_type(&ft->ResultType(i));
        }
        s << ")>";
        return s.str();
    }

    std::string print_struct_type(const jlm::structtype *st) {
        std::ostringstream s;
        s << "!llvm.struct<";
        if (st->has_name()) {
            s << "\"" << st->name() << "\"";
        }
        // Only print full declaration for non-recursive struct references
        if (!st->has_name() || std::find(struct_stack.begin(), struct_stack.end(), st->name()) == struct_stack.end()) {
            if (st->has_name()){
                this->struct_stack.push_back(st->name());
                s << ", ";
            }
            if (st->packed()) {
                s << "packed ";
            }
            s << "(";
            for (size_t i = 0; i < st->declaration()->nelements(); ++i) {
                if (i!=0) {
                    s << ", ";
                }
                s << print_type(&st->declaration()->element(i));
            }
            s << ")";
            if (st->has_name()){
                struct_stack.pop_back();
            }
        }
        s << ">";
        return s.str();
    }

    std::string print_pointer_type(const jlm::PointerType *pt) {
        std::ostringstream s;
        if (auto ft = dynamic_cast<const jlm::FunctionType*>(&pt->GetElementType())) {
            s << print_function_type(ft);
        } else {
            s << "!llvm.ptr<" << print_type(&pt->GetElementType()) << ">";
        }
        return s.str();
    }

    std::string print_array_type(const jlm::arraytype *at) {
        std::ostringstream s;
        s << "!llvm.array<" << at->nelements() << " x " << print_type(&at->element_type()) << ">";
        return s.str();
    }

    std::string print_type(const jive::type *t){
        std::ostringstream s;
        if(auto bt = dynamic_cast<const jive::bittype *>(t)){
            s << "i" << bt->nbits();
        } else if (auto loopstate_type = dynamic_cast<const jlm::loopstatetype*>(t)){
            s << "!rvsdg.loopState";
        } else if (auto iostate_type = dynamic_cast<const jlm::iostatetype*>(t)){
            s << "!rvsdg.ioState";
        } else if (auto memstate_type = dynamic_cast<const jlm::MemoryStateType*>(t)){
            s << "!rvsdg.memState";
        } else if (auto memstate_type = dynamic_cast<const jlm::MemoryStateType*>(t)){
            s << "!rvsdg.memState";
        } else if (auto vararg_type =dynamic_cast<const jlm::varargtype*>(t)) {
            s << "!jlm.varargList";
        } else if (auto pointer_type = dynamic_cast<const jlm::PointerType*>(t)){
            s << print_pointer_type(pointer_type);
        } else if (auto array_type = dynamic_cast<const jlm::arraytype*>(t)){
            s << print_array_type(array_type);
        } else if (auto struct_type = dynamic_cast<const jlm::structtype*>(t)){
            s << print_struct_type(struct_type);
        } else if (auto control_type = dynamic_cast<const jive::ctltype*>(t)){
            s << "!rvsdg.ctrl<" << control_type->nalternatives() << ">";
        } else{
            return t->debug_string();
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

    std::string print_dummy_output() {
        std::ostringstream s;
        s << "%" << output_ctr++;
        return s.str();
    }

    inline std::string print_input_origin(const jive::input *input) {
        return print_output(input->origin());
    }

    std::string print_input_with_type(const jive::input *input) {
        std::ostringstream s;
        s << print_input_origin(input) << " : " << print_type(&input->type());
        return s.str();
    }

    std::string print_apply_node(const jlm::CallNode *cn) {
        std::ostringstream s;
        s << "rvsdg.applyNode " << print_input_with_type(cn->input(0));
        s << "(";
        for (size_t i = 1; i < cn->ninputs(); ++i) {
            if(i!=1){
                s << ", ";
            }
            s << print_input_with_type(cn->input(i));
        }
        s << ")";
        if(cn->noutputs()){
            s << " -> ";
            for (size_t i = 0; i < cn->noutputs(); ++i) {
                if(i!=0){
                    s << ", ";
                }
                s << print_type(&cn->output(i)->type());
            }
        }
        return s.str();
    } 

    std::string print_constant_data_array_initialization(jive::simple_node *node, int indent_lvl) {
        assert((dynamic_cast<const jlm::ConstantDataArray*>(&node->operation())||
                dynamic_cast<const jlm::ConstantStruct*>(&node->operation())
               ) && "Can only print constant data array or constant struct nodes"
        );
        assert(this->output_map.count(node->output(0)) && "Node output must already be printed");

        std::ostringstream s;
        // Should probably somehow be replaced with "llvm.mlir.constant", but I didn't understand the documentation
        s << "llvm.mlir.undef: " << print_type(&node->output(0)->type());
        auto original_output = node->output(0);
        std::string prev_output_id = print_output(original_output);

        for (size_t i = 0; i < node->ninputs(); ++i){
            auto input = node->input(i);

            jive::simple_output ephemeral_output(node, original_output->port());

            s << "\n" << indent(indent_lvl) << print_output(&ephemeral_output) 
              << " = llvm.insertvalue " << print_input_origin(input)
              << ", " << prev_output_id << "[" << i << "]: " << print_type(&ephemeral_output.type());

            prev_output_id = print_output(&ephemeral_output);

            // Update output table to use last SSA value in the chain.
            // Also erases intermediate outputs from output_map to prevent
            // aliasing.
            this->output_map[original_output] = prev_output_id;
            this->output_map.erase(&ephemeral_output);
        }
        return s.str();
    }

    std::string print_getElementPtr_node(jive::simple_node *node) {
        assert(dynamic_cast<const jlm::getelementptr_op*>(&node->operation()) && "Can only print getElementPtr nodes");
        std::ostringstream s;
        s << "llvm.getelementptr " << print_input_origin(node->input(0)) << "[";
        for (size_t i = 1; i < node->ninputs(); ++i){
            if(i!=1){
                s << ", ";
            }
            if (auto ptr_type = dynamic_cast<const jlm::PointerType*>(&node->input(0)->type())) {
                if (auto struct_type = dynamic_cast<const jlm::structtype*>(&ptr_type->GetElementType())) {
                    s << constant_op_map[node->input(i)->origin()]->value().to_int();
                } else {
                    s << print_input_origin(node->input(i));
                }
            }
        }
        s << "]: (";
        size_t n_inputs = node->ninputs();
        if (auto ptr_type = dynamic_cast<const jlm::PointerType*>(&node->input(0)->type())) {
            if (auto struct_type = dynamic_cast<const jlm::structtype*>(&ptr_type->GetElementType())) {
                n_inputs = 1;
            }
        }
        for (size_t i = 0; i < n_inputs; ++i){
            if(i!=0){
                s << ", ";
            }
            s << print_type(&node->input(i)->type());
        }
        s << ") -> ";
        for (size_t i = 0; i < node->noutputs(); ++i){
            if(i!=0){
                s << ", ";
            }
            s << print_type(&node->output(i)->type());
        }
        return s.str();
    }

    std::string print_match_node(jive::simple_node *node, int indent_lvl = 0) {
        auto op = dynamic_cast<const jive::match_op*>(&node->operation());
        assert(op && "Can only print match nodes");
        auto rule_attr_name = "#rvsdg.matchRule";
        std::ostringstream s;
        s << "rvsdg.match(" << print_input_origin(node->input(0)) << " : " << print_type(&node->input(0)->type()) << ") [\n";
        for (auto mapping : *op) {
            s << indent(indent_lvl+1) << rule_attr_name << "<" << mapping.first << " -> " << mapping.second << ">,\n";
        }
        s << indent(indent_lvl+1) << rule_attr_name << "<default" << " -> " << op->default_alternative() << ">\n";
        s << indent(indent_lvl) << "]";
        s << " -> " << print_type(&node->output(0)->type());
        return s.str();
    }

    std::string print_alloca_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::alloca_op*>(&node->operation());
        assert (op != NULL && "Can only print alloca nodes");
        std::ostringstream s;
        s << "jlm.alloca " << print_type(&op->value_type()) << " (" ;
        for (size_t i = 1; i < node->ninputs(); ++i) {
            if(i!=1){
                s << ", ";
            }
            s << print_input_origin(node->input(i));
        }
        s << ") -> ";
        for (size_t i = 0; i < node->noutputs(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_type(&node->output(i)->type());
        }
        return s.str();
    }

    std::string print_memStateMerge_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::MemStateMergeOperator*>(&node->operation());
        assert (op != NULL && "Can only print memStateMerge nodes");
        std::ostringstream s;
        s << "rvsdg.memStateMerge (";
        for (size_t i = 0; i < node->ninputs(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_input_origin(node->input(i));
        }
        s << "): ";
        s << print_type(&node->output(0)->type()); 
        return s.str();
    }

    std::string print_bitcast_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::bitcast_op*>(&node->operation());
        assert(op != NULL && "Can only print bitcast nodes");
        std::ostringstream s;
        s << "llvm.bitcast " << print_input_origin(node->input(0)) << " : " << print_type(&node->input(0)->type()) << " to " << print_type(&node->output(0)->type());
        return s.str();
    }

    std::string print_bitcompare_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jive::bitcompare_op*>(&node->operation());
        assert(op != NULL && "Can only print bitcompare nodes");
        std::ostringstream s;
        s << "llvm.icmp \"";
        if (dynamic_cast<const jive::biteq_op*>(op)) {
            s << "eq";
        } else if (dynamic_cast<const jive::bitne_op*>(op)) {
            s << "ne";
        } else if (dynamic_cast<const jive::bitsge_op*>(op)) {
            s << "sge";
        } else if (dynamic_cast<const jive::bitsgt_op*>(op)) {
            s << "sgt";
        } else if (dynamic_cast<const jive::bitsle_op*>(op)) {
            s << "sle";
        } else if (dynamic_cast<const jive::bitslt_op*>(op)) {
            s << "slt";
        } else if (dynamic_cast<const jive::bituge_op*>(op)) {
            s << "uge";
        } else if (dynamic_cast<const jive::bitugt_op*>(op)) {
            s << "ugt";
        } else if (dynamic_cast<const jive::bitule_op*>(op)) {
            s << "ule";
        } else if (dynamic_cast<const jive::bitult_op*>(op)) {
            s << "ult";
        } else {
            assert(false && "Unknown bitcompare operation");
        }
        s << "\" " << print_input_origin(node->input(0)) << ", " << print_input_origin(node->input(1));
        s << ": " << print_type(&op->type());
        return s.str();
    }

    std::string print_load_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::LoadOperation*>(&node->operation());
        assert(op != NULL && "Can only print load nodes");
        std::ostringstream s;
        s << "jlm.load " << print_input_origin(node->input(0)) << ": " << print_type(&node->input(0)->type());
        s << " (";
        for (size_t i = 1; i < node->ninputs(); ++i) {
            if(i!=1){
                s << ", ";
            }
            s << print_input_origin(node->input(i));
        }
        s << ") -> ";
        for (size_t i = 0; i < node->noutputs(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_type(&node->output(i)->type());
        }
        return s.str();
    }

    std::string print_store_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::StoreOperation*>(&node->operation());
        assert(op != NULL && "Can only print store nodes");
        std::ostringstream s;
        s << "jlm.store (" << print_input_origin(node->input(0)) << ": " << print_type(&node->input(0)->type());
        s << ", " << print_input_origin(node->input(1)) << ": " << print_type(&node->input(1)->type());
        s << ") (";
        for (size_t i = 2; i < node->ninputs(); ++i) {
            if(i!=2){
                s << ", ";
            }
            s << print_input_origin(node->input(i));
        }
        s << ") -> ";
        for (size_t i = 0; i < node->noutputs(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_type(&node->output(i)->type());
        }
        return s.str();
    }

    std::string print_binary_bitop_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jive::bitbinary_op*>(&node->operation());
        assert(op != NULL && "Can only print binary bitop nodes");
        std::ostringstream s;
        s << "llvm.";
        if (auto addop = dynamic_cast<const jive::bitadd_op*>(op)){
            s << "add";
        } else if (auto andop = dynamic_cast<const jive::bitand_op*>(op)){
            s << "and";
        } else if (auto ashr = dynamic_cast<const jive::bitashr_op*>(op)){
            s << "ashr";
        } else if (auto mulop = dynamic_cast<const jive::bitmul_op*>(op)){
            s << "mul";
        } else if (auto orop = dynamic_cast<const jive::bitor_op*>(op)){
            s << "or";
        } else if (auto sdivop = dynamic_cast<const jive::bitsdiv_op*>(op)){
            s << "sdiv";
        } else if (auto shl = dynamic_cast<const jive::bitshl_op*>(op)){
            s << "shl";
        } else if (auto shr = dynamic_cast<const jive::bitshr_op*>(op)){
            s << "lshr";
        } else if (auto smodop = dynamic_cast<const jive::bitsmod_op*>(op)){
            s << "srem";
        } else if (auto smulhop = dynamic_cast<const jive::bitsmulh_op*>(op)){
            assert(false && "Binary bit op smulh not supported");
        } else if (auto subop = dynamic_cast<const jive::bitsub_op*>(op)){
            s << "sub";
        } else if (auto udivop = dynamic_cast<const jive::bitudiv_op*>(op)){
            s << "udiv";
        } else if (auto umodop = dynamic_cast<const jive::bitumod_op*>(op)){
            s << "urem";
        } else if (auto umulhop = dynamic_cast<const jive::bitumulh_op*>(op)){
            assert(false && "Binary bit op umulh not supported");
        } else if (auto xorop = dynamic_cast<const jive::bitxor_op*>(op)){
            s << "xor";
        } else {
            assert(false && "Unknown binary bitop");
        }
        s << " ";
        s << print_input_origin(node->input(0));
        s << ", " << print_input_origin(node->input(1));
        s << ": " << print_type(&node->output(0)->type());
        return s.str();
    }

    std::string print_memcpy_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::Memcpy*>(&node->operation());      
        assert(op != NULL && "Can only print memcpy nodes");
        std::ostringstream s;
        s << "jlm.memcpy ";
        for (size_t i = 0; i < node->ninputs(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_input_origin(node->input(i)) ;
            if (i < 4) { // Don't print types for memStates
                s<< ": " << print_type(&node->input(i)->type());
            }
        }
        s << " -> ";
        s << print_type(&node->output(0)->type());
        return s.str();
    }

    std::string print_valist_node(jive::simple_node *node) {
        auto op = dynamic_cast<const jlm::valist_op*>(&node->operation());
        assert(op != NULL && "Can only print valist nodes");
        std::ostringstream s;
        s << "jlm.createVarargs (";
        for (size_t i = 0; i < node->ninputs(); ++i) {
            if(i!=0){
                s << ", ";
            }
            s << print_input_origin(node->input(i)) << ": " << print_type(&node->input(i)->type());
        }
        s << ") -> " << print_type(&node->output(0)->type());
        return s.str();
    }

    std::string print_simple_node(jive::simple_node *node, int indent_lvl) {
        std::ostringstream s;
        if (auto o = dynamic_cast<const jive::bitconstant_op *>(&(node->operation()))) {
            auto value = o->value();
            s << "arith.constant ";
            s << value.to_int();
            s << ": " << print_type(&node->output(0)->type());
            constant_op_map[node->output(0)] = o;
        } else if (auto op = dynamic_cast<const jlm::ConstantDataArray*>(&(node->operation()))) {
            s << print_constant_data_array_initialization(node, indent_lvl);
        } else if (auto op = dynamic_cast<const jlm::ConstantStruct*>(&(node->operation()))) {
            s << print_constant_data_array_initialization(node, indent_lvl);
        } else if (auto cn = dynamic_cast<const jlm::CallNode*>(node)) {
            s << print_apply_node(cn);
        } else if (auto op = dynamic_cast<const jlm::UndefValueOperation*>(&node->operation())) {
            std::string op_name = "llvm.mlir.undef";
            if (auto type = dynamic_cast<const jive::ctltype*>(&node->output(0)->type())) {
                op_name = "jlm.undef";
            }
            s << op_name << ": " << print_type(&node->output(0)->type());
        } else if (auto op = dynamic_cast<const jlm::getelementptr_op*>(&node->operation())) {
            s << print_getElementPtr_node(node);
        } else if (is_ctlconstant_op(node->operation())) {
            auto op = to_ctlconstant_op(node->operation());
            s << "rvsdg.constantCtrl " << op.value().alternative() << ": " << print_type(&node->output(0)->type());
        } else if (auto op = dynamic_cast<const jive::match_op*>(&node->operation())) {
            s << print_match_node(node, indent_lvl);
        } else if (auto op = dynamic_cast<const jlm::alloca_op*>(&node->operation())) {
            s << print_alloca_node(node);
        } else if (auto op = dynamic_cast<const jlm::MemStateMergeOperator*>(&node->operation())) {
            s << print_memStateMerge_node(node);
        } else if (auto op = dynamic_cast<const jlm::bitcast_op*>(&node->operation())) {
            s << print_bitcast_node(node);  
        } else if (auto op = dynamic_cast<const jive::bitcompare_op*>(&node->operation())) {
            s << print_bitcompare_node(node);
        } else if (auto op = dynamic_cast<const jlm::LoadOperation*>(&node->operation())) {
            s << print_load_node(node);
        } else if (auto op = dynamic_cast<const jlm::StoreOperation*>(&node->operation())) {
            s << print_store_node(node);
        } else if (auto op = dynamic_cast<const jive::bitbinary_op*>(&node->operation())) {
            s << print_binary_bitop_node(node);
        } else if (auto op = dynamic_cast<const jlm::Memcpy*>(&node->operation())) {
            s << print_memcpy_node(node);
        } else if (auto op = dynamic_cast<const jlm::valist_op*>(&node->operation())) {
            s << print_valist_node(node);
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

    std::string print_node(jive::node *node, int indent_lvl = 0) {
        std::ostringstream s;
            s << indent(indent_lvl);
            size_t n_outputs = node->noutputs();
            // RVSDG dialect expects argument, results, and outputs to be identical.
            // Do some trickery to ensure correct number of outputs.
            if (auto phi = dynamic_cast<jlm::phi::node *>(node)) {
                n_outputs = phi->subregion()->narguments();
            }
            for (size_t i = 0; i < n_outputs; ++i) {
                if(i!=0){
                    s << ", ";
                }
                if (i < node->noutputs()){
                    s << print_output(node->output(i));
                } else {
                    s << print_dummy_output();
                }
            }
            if(node->noutputs()){
                s << " = ";
            }
            if (auto sn = dynamic_cast<jive::simple_node *>(node)) {
                s << print_simple_node(sn, indent_lvl);
            } else if (auto lambda = dynamic_cast<const jlm::lambda::node *>(node)){
                s << print_lambda(*lambda, indent_lvl);
            } else if (auto gamma = dynamic_cast<jive::gamma_node *>(node)) {
                s << print_gamma(gamma, indent_lvl);
            }  else if (auto theta = dynamic_cast<jive::theta_node *>(node)) {
                s << print_theta(theta, indent_lvl);
            } else if (auto delta = dynamic_cast<jlm::delta::node *>(node)) {
                s << print_delta(delta, indent_lvl);
            } else if (auto phi = dynamic_cast<jlm::phi::node *>(node)) {
                s << print_phi(phi, indent_lvl);
            } else {
            s << "UNIMPLEMENTED STRUCTURAL NODE: " << node->operation().debug_string() << "\n";
            // throw jlm::error("Structural node"+node->operation().debug_string()+"not implemented yet");
        }
        return s.str();
    }

    std::string print_subregion(jive::region *region, int indent_lvl, std::string result_type, int split_result_operands_at = -1, bool print_result_types = true) {
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
            s << print_node(node, indent_lvl + 1);
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
                s << print_input_origin(region->result(i));
                if (print_result_types || i >= (size_t)split_result_operands_at){
                    s << ":" << print_type(&region->result(i)->type());
                }
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
        s << "(" << print_input_origin(gn->predicate()) << ": ";
        s << print_type(&gn->predicate()->type()) << ")";
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

    std::string print_theta(const jive::theta_node *tn, int indent_lvl = 0) {
        std::ostringstream s;
        s << "rvsdg.thetaNode";
        s << "(";
        for (size_t i = 0; i < tn->ninputs(); ++i) {
            if (i != 0) {
                s << ", ";
            }
            s << print_input_origin(tn->input(i)) << ": " << print_type(&tn->input(i)->type());
        }
        s << "):\n";
        s << print_subregion(tn->subregion(), indent_lvl, "rvsdg.thetaResult", 1, false);
        s << "->";
        for (size_t i = 0; i < tn->noutputs(); ++i) {
            if (i != 0) {
                s << ", ";
            }
            s << print_type(&tn->output(i)->type());
        }
        s << "\n";
        return s.str();
    }

    std::string print_phi(const jlm::phi::node *pn, int indent_lvl = 0) {
        std::ostringstream s;
        s << "rvsdg.phiNode";
        s << "(";
        for (size_t i = 0; i < pn->ninputs(); ++i) {
            if (i != 0) {
                s << ", ";
            }
            s << print_input_origin(pn->input(i)) << ": " << print_type(&pn->input(i)->type());
        }
        s << "):\n";
        s << print_subregion(pn->subregion(), indent_lvl, "rvsdg.phiResult");
        s << "->";
        // Iterates over region arguments since RVSDG dialect expects region results, region arguments,
        // and node outputs to all match.
        for (size_t i = 0; i < pn->subregion()->narguments(); ++i) {
            if (i != 0) {
                s << ", ";
            }
            s << print_type(&pn->subregion()->argument(i)->type());
        }
        s << "\n";
        return s.str();
    }

    std::string print_delta(const jlm::delta::node *dn, int indent_lvl = 0) {
        std::ostringstream s;
        s << "rvsdg.deltaNode";
        s << "(";
        for (size_t i = 0; i < dn->ninputs(); ++i) {
            if (i != 0) {
                s << ", ";
            }
            s << print_input_origin(dn->input(i)) << ": " << print_type(&dn->input(i)->type());
        }
        s << "):\n";
        s << print_subregion(dn->subregion(), indent_lvl, "rvsdg.deltaResult");
        s << "->";
        s << print_type(&dn->output()->type());
        s << "\n";
        return s.str();
    }

    std::string print_omega(const jive::graph &graph) {
        std::ostringstream s;
        s << "rvsdg.omegaNode ";
        s << print_subregion(graph.root(), 0, "rvsdg.omegaResult");
        return s.str();
    }

public:
    std::string print_mlir(jlm::RvsdgModule &rm) {
        auto &graph = rm.Rvsdg();
        // auto root = graph.root();
        std::ostringstream s;
        // for (auto &node: root->nodes){
        //     s << print_node(&node);
        // }
        s << print_omega(graph);
        s << "\n";
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
    // jive::view(rvsdgModule->Rvsdg().root(), stderr);
    PrintMLIR printMlir;
    std::cout << printMlir.print_mlir(*rvsdgModule);

	return 0;
}
