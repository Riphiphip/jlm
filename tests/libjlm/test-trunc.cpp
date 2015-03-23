/*
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jive/evaluator/eval.h>
#include <jive/evaluator/literal.h>

#include <assert.h>

static int
verify(const jive_graph * graph)
{
	using namespace jive::evaluator;

	memliteral state;
	bitliteral xl(jive::bits::value_repr(64, 0xFFFFFFFF0000000F));

	std::unique_ptr<const literal> result;
	result = std::move(eval(graph, "test_trunc", {&xl, &state})->copy());

	const fctliteral * fctlit = dynamic_cast<const fctliteral*>(result.get());
	assert(fctlit->nresults() == 2);
	assert(dynamic_cast<const bitliteral*>(&fctlit->result(0))->value_repr() == 0x0000000F);

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/test-trunc", verify)
