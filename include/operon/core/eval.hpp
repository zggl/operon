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

#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "dataset.hpp"
#include "grammar.hpp"
#include "gsl/gsl"
#include "tree.hpp"
#include <execution>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigen>

#include <ceres/ceres.h>

namespace Operon {
constexpr gsl::index BATCHSIZE = 64;

template <typename T>
inline std::pair<T, T> MinMax(gsl::span<T> values) noexcept
{
    // get first finite (not NaN, not infinity) value
    auto min = Operon::Numeric::Max<T>();
    auto max = Operon::Numeric::Min<T>();

    for (auto v : values) {
        if (!ceres::IsFinite(v)) { continue; }
        if (min > v) { min = v; }
        if (max < v) { max = v; }
    }
    return { min, max };
}

template <typename T>
inline void LimitToRange(gsl::span<T> values, T min, T max) noexcept
{
    auto mid = (min + max) / 2.0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        auto v = values[i];
        values[i] = ceres::IsFinite(v) ? std::clamp(v, min, max) : mid;
    }
}

template <typename T>
Operon::Vector<T> Evaluate(const Tree& tree, const Dataset& dataset, const Range range, T const* const parameters = nullptr)
{
    Operon::Vector<T> result(range.Size());
    Evaluate(tree, dataset, range, parameters, gsl::span<T>(result));
    return result;
}

template <typename T>
void Evaluate(const Tree& tree, const Dataset& dataset, const Range range, T const* const parameters, gsl::span<T> result) noexcept
{
    auto& nodes = tree.Nodes();
    Eigen::Array<T, BATCHSIZE, Eigen::Dynamic, Eigen::ColMajor> m(BATCHSIZE, nodes.size());
    Eigen::Map<Eigen::Array<T, Eigen::Dynamic, 1, Eigen::ColMajor>> res(result.data(), result.size(), 1); 

    auto indices = std::vector<gsl::index>(nodes.size());
    gsl::index idx = 0;

    bool treeContainsNonlinearSymbols = false;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].IsConstant()) {
            auto v = parameters == nullptr ? T(nodes[i].Value) : parameters[idx];
            m.col(i).setConstant(v);
            idx++;
        } else if (nodes[i].IsVariable()) {
            indices[i] = dataset.GetIndex(nodes[i].HashValue);
            idx++;
        }
        treeContainsNonlinearSymbols |= static_cast<bool>(nodes[i].Type & ~Grammar::Arithmetic);
    }

    auto lastCol = m.col(nodes.size()-1);

    auto const& values = dataset.Values();

    gsl::index numRows = range.Size();
    for (gsl::index row = 0; row < numRows; row += BATCHSIZE) {
        idx = 0;
        auto remainingRows = std::min(BATCHSIZE, numRows - row);
        for (size_t i = 0; i < nodes.size(); ++i) {
            auto r = m.col(i);

            switch (auto const& s = nodes[i]; s.Type) {
            case NodeType::Add: {
                auto c1 = i - 1; // first child index
                auto c2 = c1 - 1 - nodes[c1].Length;
                r = m.col(c1) + m.col(c2);
                break;
            }
            case NodeType::Mul: {
                auto c1 = i - 1; // first child index
                auto c2 = c1 - 1 - nodes[c1].Length;
                r = m.col(c1) * m.col(c2);
                break;
            }
            case NodeType::Sub: {
                auto c1 = i - 1; // first child index
                auto c2 = c1 - 1 - nodes[c1].Length;
                r = m.col(c1) - m.col(c2);
                break;
            }
            case NodeType::Div: {
                auto c1 = i - 1; // first child index
                auto c2 = c1 - 1 - nodes[c1].Length;
                r = m.col(c1) / m.col(c2);
                break;
            }
            case NodeType::Constant: {
                idx++;
                break;
            }
            case NodeType::Variable: {
                auto w = parameters == nullptr ? T(s.Value) : parameters[idx++];
                r.segment(0, remainingRows) = w * values.col(indices[i]).segment(range.Start() + row, remainingRows).cast<T>();
                break;
            }
            default: {
                if (treeContainsNonlinearSymbols) {
                    switch (s.Type) {
                        case NodeType::Log: {
                            r = m.col(i - 1).log();
                            break;
                        }
                        case NodeType::Exp: {
                            r = m.col(i - 1).exp();
                            break;
                        }
                        case NodeType::Sin: {
                            r = m.col(i - 1).sin();
                            break;
                        }
                        case NodeType::Cos: {
                            r = m.col(i - 1).cos();
                            break;
                        }
                        case NodeType::Tan: {
                            r = m.col(i - 1).tan();
                            break;
                        }
                        case NodeType::Sqrt: {
                            r = m.col(i - 1).sqrt();
                            break;
                        }
                        case NodeType::Cbrt: {
                            r = m.col(i - 1).unaryExpr([](T v) { return T(ceres::cbrt(v)); });
                            break;
                        }
                        case NodeType::Square: {
                            r = m.col(i - 1).square();
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                break;
                //fmt::print(stderr, "Unknown node type {}\n", nodes[i].Name());
                //std::terminate();
            }
            }
        }
        // the final result is found in the last section of the buffer corresponding to the root node
        res.segment(row, remainingRows) = lastCol.segment(0, remainingRows).unaryExpr([](T v) { return ceres::IsFinite(v) ? v : Operon::Numeric::Max<T>(); });
    }
}

struct TreeEvaluator {
    TreeEvaluator(const Tree& tree, const Dataset& dataset, const Range range)
        : tree_ref(tree)
        , dataset_ref(dataset)
        , range(range)
    {
    }

    template <typename T>
    bool operator()(T const* const* parameters, T* residuals) const
    {
        auto res = gsl::span<T>(residuals, range.Size());
        Evaluate(tree_ref, dataset_ref, range, parameters[0], res);
        return true;
    }

private:
    std::reference_wrapper<const Tree> tree_ref;
    std::reference_wrapper<const Dataset> dataset_ref;
    Range range;
};

struct ResidualEvaluator {
    ResidualEvaluator(const Tree& tree, const Dataset& dataset, const gsl::span<const Operon::Scalar> targetValues, const Range range)
        : treeEvaluator(tree, dataset, range)
        , target_ref(targetValues)
    {
    }

    template <typename T>
    bool operator()(T const* const* parameters, T* residuals) const
    {
        treeEvaluator(parameters, residuals);
        Eigen::Map<Eigen::Array<T, Eigen::Dynamic, 1, Eigen::ColMajor>> resMap(residuals, target_ref.size());
        Eigen::Map<const Eigen::Array<Operon::Scalar, Eigen::Dynamic, 1, Eigen::ColMajor>> targetMap(target_ref.data(), target_ref.size());
        resMap -= targetMap.cast<T>();
        return true;
    }

private:
    TreeEvaluator treeEvaluator;
    gsl::span<const Operon::Scalar> target_ref;
};
}
#endif

