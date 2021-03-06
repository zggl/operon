/* This code represents derived work from ELKI:
 * Environment for Developing KDD-Applications Supported by Index-Structures
 *
 * Copyright (C) 2019
 * ELKI Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "stat/meanvariance.hpp"

namespace Operon {
    void MeanVarianceCalculator::Add(Operon::Scalar val) 
    {
        if (n <= 0) {
            n = 1;
            sum = val;
            m2 = 0;
            return;
        }
        double tmp = n * val - sum;
        double oldn = n; // tmp copy
        n += 1.0;
        sum += val;
        m2 += tmp * tmp / (n * oldn);
    }

    void MeanVarianceCalculator::Add(Operon::Scalar val, Operon::Scalar weight) 
    {
        if (weight == 0.) {
            return;
        }
        if (n <= 0) {
            n = weight;
            sum = val * weight;
            return;
        }
        val *= weight;
        double tmp = n * val - sum * weight;
        double oldn = n; // tmp copy
        n += weight;
        sum += val;
        m2 += tmp * tmp / (weight * n * oldn);
    }

    void MeanVarianceCalculator::Add(gsl::span<Operon::Scalar> vals) 
    {
        int l = vals.size();
        if (l < 2) {
            if (l == 1) {
                Add(vals[0]);
            }
            return;
        }
        // First pass:
        double s1 = 0.;
        for (int i = 0; i < l; i++) {
            s1 += vals[i];
        }
        double om1 = s1 / l;
        // Second pass:
        double om2 = 0., err = 0.;
        for (int i = 0; i < l; i++) {
            double v = vals[i] - om1;
            om2 += v * v;
            err += v;
        }
        s1 += err;
        om2 += err / l;
        if (n <= 0) {
            n = l;
            sum = s1;
            m2 = om2;
            return;
        }
        double tmp = n * s1 - sum * l;
        double oldn = n; // tmp copy
        n += l;
        sum += s1 + err;
        m2 += om2 + tmp * tmp / (l * n * oldn);
    }

    void MeanVarianceCalculator::Add(gsl::span<Operon::Scalar> vals, gsl::span<Operon::Scalar> weights) 
    {
        Expects(vals.size() == weights.size());
        for (int i = 0, end = vals.size(); i < end; i++) {
            // TODO: use a two-pass update as in the other put
            Add(vals[i], weights[i]);
        }
    }
}
