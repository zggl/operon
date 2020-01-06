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

#ifndef OS_GENERATOR_HPP
#define OS_GENERATOR_HPP

#include "core/operator.hpp"

namespace Operon {
template <typename TEvaluator, typename TSelector, typename TCrossover, typename TMutator>
class OffspringSelectionGenerator : public OffspringGeneratorBase<TEvaluator, TSelector, TCrossover, TMutator> {
public:
    explicit OffspringSelectionGenerator(TEvaluator& eval, TSelector& sel, TCrossover& cx, TMutator& mut)
        : OffspringGeneratorBase<TEvaluator, TSelector, TCrossover, TMutator>(eval, sel, cx, mut)
          , basicGenerator(eval, sel, cx, mut)
    {
    }

    using T = typename TSelector::SelectableType;
    std::optional<T> operator()(Operon::Random& random, double pCrossover, double pMutation) const override
    {
        std::uniform_real_distribution<double> uniformReal;
        bool doCrossover = uniformReal(random) < pCrossover;
        bool doMutation = uniformReal(random) < pMutation;

        if (!(doCrossover || doMutation))
            return std::nullopt;

        constexpr gsl::index Idx = TSelector::SelectableIndex;
        auto population = this->Selector().Population();

        auto first = this->selector(random);
        auto fit = population[first][Idx];

        typename TSelector::SelectableType child;

        if (doCrossover) {
            auto second = this->selector(random);
            child.Genotype = this->crossover(random, population[first].Genotype, population[second].Genotype);

            fit = std::min(fit, population[second][Idx]);
        }

        if (doMutation) {
            child.Genotype = doCrossover
                ? this->mutator(random, std::move(child.Genotype))
                : this->mutator(random, population[first].Genotype);
        }

        auto f = this->evaluator(random, child);

        if (std::isfinite(f) && f < fit) {
            child[Idx] = this->evaluator(random, child);
            return std::make_optional(child);
        }
        return std::nullopt;
    }

    void MaxSelectionPressure(size_t value) { maxSelectionPressure = value; }
    size_t MaxSelectionPressure() const { return maxSelectionPressure; }

    void Prepare(const gsl::span<const T> pop) const override
    {
        OffspringGeneratorBase<TEvaluator, TSelector, TCrossover, TMutator>::Prepare(pop);
        lastEvaluations = this->evaluator.get().FitnessEvaluations();
    }

    double SelectionPressure() const
    {
        if (this->Selector().Population().empty()) {
            return 0;
        }
        return (this->evaluator.get().FitnessEvaluations() - lastEvaluations) / static_cast<double>(this->Selector().Population().size());
    }

    bool Terminate() const override
    {
        return OffspringGeneratorBase<TEvaluator, TSelector, TCrossover, TMutator>::Terminate() || SelectionPressure() > maxSelectionPressure;
    };

private:
    mutable size_t lastEvaluations;
    size_t maxSelectionPressure;
    BasicOffspringGenerator<TEvaluator, TSelector, TCrossover, TMutator> basicGenerator;
};
} // namespace Operon

#endif