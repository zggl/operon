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

#include <catch2/catch.hpp>

#include "core/tree.hpp"
#include "core/common.hpp"
#include "core/operator.hpp"

#include <ceres/ceres.h>

namespace Operon {
namespace Test {
TEST_CASE("Node is trivial", "[detail]")
{
    REQUIRE(std::is_trivial_v<Operon::Node>);
}

TEST_CASE("Node is trivially copyable", "[detail]")
{
    REQUIRE(std::is_trivially_copyable_v<Operon::Node>);
}

TEST_CASE("Node is standard layout", "[detail]")
{
    REQUIRE(std::is_standard_layout_v<Operon::Node>);
}

TEST_CASE("Node is pod", "[detail]")
{
    REQUIRE(std::is_pod_v<Operon::Node>);
}

TEST_CASE("Node is small", "[detail]")
{
    // this test case basically wants to ensure that, 
    // for memory efficiency purposes, the Node struct
    // is kept as small as possible
    auto node                  = static_cast<Node*>(nullptr);
    auto szType                = sizeof(node->Type);
    auto szArity               = sizeof(node->Arity);
    auto szLength              = sizeof(node->Length);
    auto szDepth               = sizeof(node->Depth);
    auto szEnabled             = sizeof(node->IsEnabled);
    auto szHashValue           = sizeof(node->HashValue);
    auto szCalculatedHashValue = sizeof(node->CalculatedHashValue);
    auto szValue               = sizeof(node->Value);
    auto szParent              = sizeof(node->Parent);
    fmt::print("Size breakdown of the Node class:\n");
    fmt::print("Type                {:>2}\n", szType);
    fmt::print("Arity               {:>2}\n", szArity);
    fmt::print("Length              {:>2}\n", szLength);
    fmt::print("Depth               {:>2}\n", szDepth);
    fmt::print("Parent              {:>2}\n", szParent);
    fmt::print("Enabled             {:>2}\n", szEnabled);
    fmt::print("Value               {:>2}\n", szValue);
    fmt::print("HashValue           {:>2}\n", szHashValue);
    fmt::print("CalculatedHashValue {:>2}\n", szCalculatedHashValue);
    fmt::print("-------------------------\n");
    auto szTotal = szType + szArity + szLength + szDepth + szEnabled + szHashValue + szParent + szCalculatedHashValue + szValue;
    fmt::print("Total               {:>2}\n", szTotal); 
    fmt::print("Total + padding     {:>2}\n", sizeof(Node));
    fmt::print("-------------------------\n");
    std::vector<Node> nodes;
    std::generate_n(std::back_inserter(nodes), 50, [](){ return Node(NodeType::Add); });
    Tree tree { nodes };
    fmt::print("sizeof(Tree)        {:>2}\n", sizeof(tree));
    fmt::print("sizeof(vector<Node>) {:>2}\n", sizeof(nodes));
    Individual<1> ind { tree, { 0 } };
    fmt::print("sizeof(Individual)  {:>2}\n", sizeof(ind));


    REQUIRE(sizeof(Node) <= size_t{64});
}

TEST_CASE("Jsf is copyable", "[detail]") 
{
    RandomGenerator::JsfRand<64> jsf(1234);
    jsf();
    jsf();
    auto tmp = jsf;

    REQUIRE(tmp() == jsf());
}

TEST_CASE("Sfc64 is copyable", "[detail]")
{
    RandomGenerator::Sfc64 sfc(1234);
    sfc();
    sfc();
    auto tmp = sfc;

    REQUIRE(tmp() == sfc());
}

TEST_CASE("Jet is floating-point", "[detail]") 
{
    std::cout << Operon::Numeric::Min<double>() << "\n";
    std::cout << Operon::Numeric::Min<Operon::Dual>() << "\n";
    std::cout << std::numeric_limits<ceres::Jet<double, 4>>::lowest() << "\n";
}
} // namespace Test
} // namespace Operon
