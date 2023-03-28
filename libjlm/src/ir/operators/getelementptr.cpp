/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/ir/operators/getelementptr.hpp>

namespace jlm {

/* getelementptr operator */

getelementptr_op::~getelementptr_op()
{}

getelementptr_op::getelementptr_op(const getelementptr_op & other)
  : jive::simple_op(*this)
  , PointeeType_(other.PointeeType_->copy())
{}

getelementptr_op::getelementptr_op(getelementptr_op && other) noexcept
  : jive::simple_op(*this)
  , PointeeType_(std::move(other.PointeeType_))
{}

bool
getelementptr_op::operator==(const operation & other) const noexcept
{
  auto op = dynamic_cast<const jlm::getelementptr_op*>(&other);
  if (!op || op->narguments() != narguments())
    return false;

  if (*PointeeType_ != *op->PointeeType_)
    return false;

  for (size_t n = 0; n < narguments(); n++) {
    if (op->argument(n) != argument(n))
      return false;
  }

  return op->result(0) == result(0);
}

std::string
getelementptr_op::debug_string() const
{
	return "GETELEMENTPTR";
}

std::unique_ptr<jive::operation>
getelementptr_op::copy() const
{
	return std::unique_ptr<jive::operation>(new getelementptr_op(*this));
}

}
