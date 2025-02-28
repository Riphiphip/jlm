/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_OPT_INLINE_HPP
#define JLM_OPT_INLINE_HPP

#include <jlm/opt/optimization.hpp>
#include <jlm/ir/operators/lambda.hpp>

namespace jlm {

class RvsdgModule;

/**
* \brief Function Inlining
*/
class fctinline final : public optimization {
public:
	virtual
	~fctinline();

	virtual void
	run(
    RvsdgModule & module,
    StatisticsCollector & statisticsCollector) override;
};

jive::output *
find_producer(jive::input * input);

void
inlineCall(jive::simple_node * call, const lambda::node * lambda);

}

#endif
