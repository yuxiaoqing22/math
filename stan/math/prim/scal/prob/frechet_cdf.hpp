#ifndef STAN_MATH_PRIM_SCAL_PROB_FRECHET_CDF_HPP
#define STAN_MATH_PRIM_SCAL_PROB_FRECHET_CDF_HPP

#include <boost/random/weibull_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <stan/math/prim/scal/meta/operands_and_partials.hpp>
#include <stan/math/prim/scal/err/check_consistent_sizes.hpp>
#include <stan/math/prim/scal/err/check_nonnegative.hpp>
#include <stan/math/prim/scal/err/check_not_nan.hpp>
#include <stan/math/prim/scal/err/check_positive.hpp>
#include <stan/math/prim/scal/err/check_positive_finite.hpp>
#include <stan/math/prim/scal/fun/log1m.hpp>
#include <stan/math/prim/scal/fun/multiply_log.hpp>
#include <stan/math/prim/scal/fun/value_of.hpp>
#include <stan/math/prim/scal/meta/length.hpp>
#include <stan/math/prim/scal/meta/is_constant_struct.hpp>
#include <stan/math/prim/scal/meta/scalar_seq_view.hpp>
#include <stan/math/prim/scal/meta/VectorBuilder.hpp>
#include <stan/math/prim/scal/meta/partials_return_type.hpp>
#include <stan/math/prim/scal/meta/return_type.hpp>
#include <stan/math/prim/scal/fun/constants.hpp>
#include <stan/math/prim/scal/meta/include_summand.hpp>
#include <cmath>
#include <string>

namespace stan {
namespace math {

template <typename T_y, typename T_shape, typename T_scale>
typename return_type<T_y, T_shape, T_scale>::type frechet_cdf(
    const T_y& y, const T_shape& alpha, const T_scale& sigma) {
  typedef typename stan::partials_return_type<T_y, T_shape, T_scale>::type
      T_partials_return;

  static const std::string function = "frechet_cdf";

  using boost::math::tools::promote_args;
  using std::log;
  using std::exp;

  if (!(stan::length(y) && stan::length(alpha) && stan::length(sigma)))
    return 1.0;

  T_partials_return cdf(1.0);
  check_positive(function, "Random variable", y);
  check_positive_finite(function, "Shape parameter", alpha);
  check_positive_finite(function, "Scale parameter", sigma);

  operands_and_partials<T_y, T_shape, T_scale> ops_partials(y, alpha, sigma);

  scalar_seq_view<T_y> y_vec(y);
  scalar_seq_view<T_scale> sigma_vec(sigma);
  scalar_seq_view<T_shape> alpha_vec(alpha);
  size_t N = max_size(y, sigma, alpha);
  for (size_t n = 0; n < N; n++) {
    const T_partials_return y_dbl = value_of(y_vec[n]);
    const T_partials_return sigma_dbl = value_of(sigma_vec[n]);
    const T_partials_return alpha_dbl = value_of(alpha_vec[n]);
    const T_partials_return pow_ = pow(sigma_dbl / y_dbl, alpha_dbl);
    const T_partials_return cdf_ = exp(-pow_);

    cdf *= cdf_;

    if (!is_constant_struct<T_y>::value)
      ops_partials.edge1_.partials_[n] += pow_ * alpha_dbl / y_dbl;
    if (!is_constant_struct<T_shape>::value)
      ops_partials.edge2_.partials_[n] += pow_ * log(y_dbl / sigma_dbl);
    if (!is_constant_struct<T_scale>::value)
      ops_partials.edge3_.partials_[n] -= pow_ * alpha_dbl / sigma_dbl;
  }

  if (!is_constant_struct<T_y>::value) {
    for (size_t n = 0; n < stan::length(y); ++n)
      ops_partials.edge1_.partials_[n] *= cdf;
  }
  if (!is_constant_struct<T_shape>::value) {
    for (size_t n = 0; n < stan::length(alpha); ++n)
      ops_partials.edge2_.partials_[n] *= cdf;
  }
  if (!is_constant_struct<T_scale>::value) {
    for (size_t n = 0; n < stan::length(sigma); ++n)
      ops_partials.edge3_.partials_[n] *= cdf;
  }
  return ops_partials.build(cdf);
}

}  // namespace math
}  // namespace stan
#endif
