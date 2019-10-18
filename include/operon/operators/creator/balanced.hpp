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

#ifndef BALANCED_TREE_CREATOR_HPP
#define BALANCED_TREE_CREATOR_HPP 

#include <algorithm>
#include <execution>
#include <stack>

#include "core/grammar.hpp"
#include "core/operator.hpp"

namespace Operon {

// this tree creator cares about the shape of the resulting tree and tries to 
// build balanced trees by splitting the available length between child nodes 
// the creator will follow a given distribution of tree sizes but due to shape
// restrictions it can't guarantee desired symbol frequencies (TODO)
template <typename T>
class BalancedTreeCreator : public CreatorBase {
public:
    BalancedTreeCreator(T distribution, size_t depth, size_t length)
        : dist(distribution.param())
        , maxDepth(depth)
        , maxLength(length)
    {
    }
    Tree operator()(operon::rand_t& random, const Grammar& grammar, const gsl::span<const Variable> variables) const override
    {
        std::vector<Node> nodes;
        std::stack<std::tuple<Node, size_t, size_t, size_t>> stk;

        std::uniform_int_distribution<size_t> uniformInt(0, variables.size() - 1);
        std::normal_distribution<double> normalReal(0, 1);

        size_t minLength = 1u; // a leaf as root node
        size_t targetLen = std::clamp(SampleFromDistribution(random), minLength, maxLength);
        Expects(targetLen > 0);

        auto [grammarMinArity, grammarMaxArity] = grammar.FunctionArityLimits();

        auto minArity = std::min(grammarMinArity, targetLen - 1);
        auto maxArity = std::min(grammarMaxArity, targetLen - 1);

        auto init = [&](Node& node) {
            if (node.IsVariable()) {
                node.HashValue = node.CalculatedHashValue = variables[uniformInt(random)].Hash;
            }
            node.Value = normalReal(random);
        };

        auto root = grammar.SampleRandomSymbol(random, minArity, maxArity);
        init(root);

        targetLen = targetLen - 1; // because we already added 1 root node
        size_t openSlots = root.Arity;
        stk.emplace(root, root.Arity, 1, targetLen); // node, slot, depth, available length

        auto childLen = 0ul;
        while (!stk.empty()) {
            auto [node, slot, depth, length] = stk.top();
            stk.pop();

            if (slot == 0) {
                nodes.push_back(node); // this node's children have been filled
                continue;
            }
            stk.emplace(node, slot - 1, depth, length);

            childLen = slot == node.Arity ? length % node.Arity : childLen;
            childLen += length / node.Arity - 1;

            maxArity = depth == maxDepth - 1u ? 0u : std::min(grammarMaxArity, std::min(childLen, targetLen - openSlots));
            minArity = std::min(grammarMinArity, maxArity);
            auto child = grammar.SampleRandomSymbol(random, minArity, maxArity);
            init(child);

            targetLen = targetLen - 1;
            openSlots = openSlots + child.Arity - 1;

            stk.emplace(child, child.Arity, depth + 1, childLen);

        }
        auto tree = Tree(nodes).UpdateNodes();
        return tree;
    }

private:
    mutable T dist;
    size_t maxDepth;
    size_t maxLength;

    inline size_t SampleFromDistribution(operon::rand_t& random) const
    {
        auto val = dist(random);
        if constexpr (std::is_floating_point_v<typename T::result_type>) {
            val = static_cast<size_t>(std::round(val));
        }
        return val;
    }
};
}
#endif