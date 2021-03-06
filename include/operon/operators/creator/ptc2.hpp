/* This file is part of:
 * Operon - Large Scale Genetic Programming Framework
 *
 * Licensed under the ISC License <https://opensource.org/licenses/ISC> 
 * Copyright (C) 2019 Bogdan Burlacu 
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE. 
 */

#ifndef PROBABILISTIC_TREE_CREATOR_HPP
#define PROBABILISTIC_TREE_CREATOR_HPP

#include <map>
#include <unordered_map>
#include <vector>
#include <queue>
#include <deque>

#include "core/grammar.hpp"
#include "core/operator.hpp"

namespace Operon {

class ProbabilisticTreeCreator : public CreatorBase {
public:

    ProbabilisticTreeCreator(const Grammar& grammar, const gsl::span<const Variable> variables)
        : CreatorBase(grammar, variables)
    {
    }
    Tree operator()(Operon::Random& random, size_t targetLen, size_t maxDepth) const override;
};
}

#endif
