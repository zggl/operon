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

#include "core/dataset.hpp"
#include "core/eval.hpp"
#include "core/nnls.hpp"
#include "core/format.hpp"
#include "core/stats.hpp"
#include "core/metrics.hpp"

#include <catch2/catch.hpp>

#include "core/nnls_tiny.hpp"
#include <ceres/tiny_solver.h>

namespace Operon {
namespace Test {
TEST_CASE("Evaluation correctness", "[implementation]")
{
    auto ds = Dataset("../data/Poly-10.csv", true);
    auto variables = ds.Variables();

    auto range = Range { 0, 10 };
    auto targetValues = ds.GetValues("Y").subspan(range.Start(), range.Size());

    auto x1Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X1"; });
    auto x2Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X2"; });
    auto x3Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X3"; });
    auto x4Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X4"; });
    auto x5Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X5"; });
    auto x6Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X6"; });

    auto x1 = Node(NodeType::Variable, x1Var.Hash);
    x1.Value = 1;
    auto x2 = Node(NodeType::Variable, x2Var.Hash);
    x2.Value = 1;
    auto x3 = Node(NodeType::Variable, x3Var.Hash);
    x3.Value = -0.018914965743;
    auto x4 = Node(NodeType::Variable, x4Var.Hash);
    x4.Value = 1;
    auto x5 = Node(NodeType::Variable, x5Var.Hash);
    x5.Value = 0.876406042248;
    auto x6 = Node(NodeType::Variable, x6Var.Hash);
    x6.Value = 0.518227954421;

    auto add = Node(NodeType::Add);
    auto sub = Node(NodeType::Sub);
    auto mul = Node(NodeType::Mul);
    auto div = Node(NodeType::Div);

    Tree tree;

    SECTION("Addition")
    {
        auto x1Values = ds.Values().col(x1Var.Index).segment(range.Start(), range.Size());
        auto x2Values = ds.Values().col(x2Var.Index).segment(range.Start(), range.Size());

        auto res = x1Values.array() + x2Values.array();

        tree = Tree({ x1, x2, add });
        auto estimatedValues = Evaluate<Operon::Scalar>(tree, ds, range);

        for (size_t i = 0; i < targetValues.size(); ++i) {
            fmt::print("{}\t{}\t{}\t{}\n", x1Values[i], x2Values[i], res(i), estimatedValues[i]);
        }
    }

    SECTION("Subtraction")
    {
        auto x1Values = ds.Values().col(x1Var.Index).segment(range.Start(), range.Size());
        auto x2Values = ds.Values().col(x2Var.Index).segment(range.Start(), range.Size());

        auto res = x2Values.array() - x1Values.array();

        tree = Tree({ x1, x2, sub });
        auto estimatedValues = Evaluate<Operon::Scalar>(tree, ds, range);

        for (size_t i = 0; i < targetValues.size(); ++i) {
            fmt::print("{}\t{}\t{}\t{}\n", x1Values[i], x2Values[i], res(i), estimatedValues[i]);
        }
    }

    SECTION("Multiplication")
    {
        auto x1Values = ds.GetValues(x1Var.Hash).subspan(range.Start(), range.Size());
        auto x2Values = ds.GetValues(x2Var.Hash).subspan(range.Start(), range.Size());

        tree = Tree({ x1, x2, mul });
        auto estimatedValues = Evaluate<Operon::Scalar>(tree, ds, range);
        auto r2 = RSquared(estimatedValues, targetValues);
        fmt::print("{} r2 = {}\n", InfixFormatter::Format(tree, ds), r2);

        for (size_t i = 0; i < estimatedValues.size(); ++i) {
            fmt::print("{}\t{}\t{}\n", x1Values[i], x2Values[i], estimatedValues[i]);
        }
    }

    SECTION("Division")
    {
        auto x1Values = ds.GetValues(x1Var.Hash).subspan(range.Start(), range.Size());
        auto x2Values = ds.GetValues(x2Var.Hash).subspan(range.Start(), range.Size());

        tree = Tree { x1, x2, div };
        auto estimatedValues = Evaluate<Operon::Scalar>(tree, ds, range);
        auto r2 = RSquared(estimatedValues, targetValues);
        fmt::print("{} r2 = {}\n", InfixFormatter::Format(tree, ds), r2);

        for (size_t i = 0; i < estimatedValues.size(); ++i) {
            fmt::print("{}\t{}\t{}\n", x1Values[i], x2Values[i], estimatedValues[i]);
        }
    }

    SECTION("((0.876405277537 * X5 * 0.518227954421 * X6) - (-0.018914965743) * X3)")
    {
        tree = Tree { x3, x6, x5, mul, sub };

        auto x3Values = ds.GetValues(x3Var.Hash).subspan(range.Start(), range.Size());
        auto x5Values = ds.GetValues(x5Var.Hash).subspan(range.Start(), range.Size());
        auto x6Values = ds.GetValues(x6Var.Hash).subspan(range.Start(), range.Size());

        auto estimatedValues = Evaluate<Operon::Scalar>(tree, ds, range);
        auto r2 = RSquared(estimatedValues, targetValues);
        fmt::print("{} r2 = {}\n", InfixFormatter::Format(tree, ds, 12), r2);

        for (size_t i = 0; i < estimatedValues.size(); ++i) {
            fmt::print("{}\t{}\t{}\t{}\n", x3Values[i], x5Values[i], x6Values[i], estimatedValues[i]);
        }
    }
}

TEST_CASE("Constant optimization (autodiff)", "[implementation]")
{
    auto ds = Dataset("../data/Poly-10.csv", true);
    auto variables = ds.Variables();

    auto range = Range { 0, 250 };
    auto targetValues = ds.GetValues("Y").subspan(range.Start(), range.Size());

    auto x1Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X1"; });
    auto x2Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X2"; });
    auto x3Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X3"; });
    auto x4Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X4"; });
    auto x5Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X5"; });
    auto x6Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X6"; });
    auto x7Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X7"; });
    auto x8Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X8"; });
    auto x9Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X9"; });
    auto x10Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X10"; });

    auto x1 = Node(NodeType::Variable, x1Var.Hash);
    x1.Value = 0.001;
    auto x2 = Node(NodeType::Variable, x2Var.Hash);
    x2.Value = 0.001;
    auto x3 = Node(NodeType::Variable, x3Var.Hash);
    x3.Value = 0.001;
    auto x4 = Node(NodeType::Variable, x4Var.Hash);
    x4.Value = 0.001;
    auto x5 = Node(NodeType::Variable, x5Var.Hash);
    x5.Value = 0.001;
    auto x6 = Node(NodeType::Variable, x6Var.Hash);
    x6.Value = 0.001;
    auto x7 = Node(NodeType::Variable, x7Var.Hash);
    x7.Value = 0.001;
    auto x9 = Node(NodeType::Variable, x9Var.Hash);
    x9.Value = 0.001;
    auto x10 = Node(NodeType::Variable, x10Var.Hash);
    x10.Value = 0.001;

    auto add = Node(NodeType::Add);
    auto mul = Node(NodeType::Mul);

    auto poly10 = Tree {
        x1,
        x2,
        mul,
        x3,
        x4,
        mul,
        add,
        x5,
        x6,
        mul,
        add,
        x1,
        x7,
        mul,
        x9,
        mul,
        add,
        x3,
        x6,
        mul,
        x10,
        mul,
        add,
    };
    poly10.UpdateNodes();
    fmt::print("{}\n", InfixFormatter::Format(poly10, ds, 6));

    auto coef = OptimizeAutodiff(poly10, ds, targetValues, range, 100, true, true);
    fmt::print("{}\n", InfixFormatter::Format(poly10, ds, 6));
}

TEST_CASE("Constant optimization (tiny solver)", "[implementation]") 
{
    auto ds = Dataset("../data/Poly-10.csv", true);
    auto variables = ds.Variables();

    auto range = Range { 0, 250 };
    auto targetValues = ds.GetValues("Y").subspan(range.Start(), range.Size());

    auto x1Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X1"; });
    auto x2Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X2"; });
    auto x3Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X3"; });
    auto x4Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X4"; });
    auto x5Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X5"; });
    auto x6Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X6"; });
    auto x7Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X7"; });
    auto x8Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X8"; });
    auto x9Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X9"; });
    auto x10Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X10"; });

    auto x1 = Node(NodeType::Variable, x1Var.Hash);
    x1.Value = 0.001;
    auto x2 = Node(NodeType::Variable, x2Var.Hash);
    x2.Value = 0.001;
    auto x3 = Node(NodeType::Variable, x3Var.Hash);
    x3.Value = 0.001;
    auto x4 = Node(NodeType::Variable, x4Var.Hash);
    x4.Value = 0.001;
    auto x5 = Node(NodeType::Variable, x5Var.Hash);
    x5.Value = 0.001;
    auto x6 = Node(NodeType::Variable, x6Var.Hash);
    x6.Value = 0.001;
    auto x7 = Node(NodeType::Variable, x7Var.Hash);
    x7.Value = 0.001;
    auto x9 = Node(NodeType::Variable, x9Var.Hash);
    x9.Value = 0.001;
    auto x10 = Node(NodeType::Variable, x10Var.Hash);
    x10.Value = 0.001;

    auto add = Node(NodeType::Add);
    auto mul = Node(NodeType::Mul);

    auto poly10 = Tree {
        x1,
        x2,
        mul,
        x3,
        x4,
        mul,
        add,
        x5,
        x6,
        mul,
        add,
        x1,
        x7,
        mul,
        x9,
        mul,
        add,
        x3,
        x6,
        mul,
        x10,
        mul,
        add,
    };
    poly10.UpdateNodes();
    fmt::print("{}\n", InfixFormatter::Format(poly10, ds, 6));

    auto coeff = poly10.GetCoefficients();
    Eigen::Matrix<double, Eigen::Dynamic, 1> x0(coeff.size());

    for (size_t i = 0; i < coeff.size(); ++i) {
        x0(i) = coeff[i];
    }

    std::cout << "x0: " << x0.transpose() << "\n";

    auto target = ds.GetValues("Y").subspan(range.Start(), range.Size());

    ceres::TinySolver<TinyCostFunction> solver;
    TinyCostFunction function(poly10, ds, target, range);
    auto summary = solver.Solve(function, &x0); 

    std::cout << "x_final: " << x0.transpose() << "\n";
}

TEST_CASE("Constant optimization (numeric)", "[implementation]")
{
    auto ds = Dataset("../data/Poly-10.csv", true);
    auto variables = ds.Variables();

    auto range = Range { 0, 250 };
    auto targetValues = ds.GetValues("Y").subspan(range.Start(), range.Size());

    auto x1Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X1"; });
    auto x2Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X2"; });
    auto x3Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X3"; });
    auto x4Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X4"; });
    auto x5Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X5"; });
    auto x6Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X6"; });
    auto x7Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X7"; });
    auto x8Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X8"; });
    auto x9Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X9"; });
    auto x10Var = *std::find_if(variables.begin(), variables.end(), [](auto& v) { return v.Name == "X10"; });

    auto x1 = Node(NodeType::Variable, x1Var.Hash);
    x1.Value = 0.001;
    auto x2 = Node(NodeType::Variable, x2Var.Hash);
    x2.Value = 0.001;
    auto x3 = Node(NodeType::Variable, x3Var.Hash);
    x3.Value = 0.001;
    auto x4 = Node(NodeType::Variable, x4Var.Hash);
    x4.Value = 0.001;
    auto x5 = Node(NodeType::Variable, x5Var.Hash);
    x5.Value = 0.001;
    auto x6 = Node(NodeType::Variable, x6Var.Hash);
    x6.Value = 0.001;
    auto x7 = Node(NodeType::Variable, x7Var.Hash);
    x7.Value = 0.001;
    auto x9 = Node(NodeType::Variable, x9Var.Hash);
    x9.Value = 0.001;
    auto x10 = Node(NodeType::Variable, x10Var.Hash);
    x10.Value = 0.001;

    auto add = Node(NodeType::Add);
    auto mul = Node(NodeType::Mul);

    auto poly10 = Tree {
        x1,
        x2,
        mul,
        x3,
        x4,
        mul,
        add,
        x5,
        x6,
        mul,
        add,
        x1,
        x7,
        mul,
        x9,
        mul,
        add,
        x3,
        x6,
        mul,
        x10,
        mul,
        add,
    };
    poly10.UpdateNodes();
    fmt::print("{}\n", InfixFormatter::Format(poly10, ds, 6));

    auto coef = OptimizeNumeric(poly10, ds, targetValues, range, 100, true, true);
    fmt::print("{}\n", InfixFormatter::Format(poly10, ds, 6));
}

} // namespace Test
} // namespace Operon

