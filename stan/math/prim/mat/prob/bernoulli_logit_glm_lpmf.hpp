#ifndef STAN_MATH_PRIM_MAT_PROB_BERNOULLI_LOGIT_GLM_LPMF_HPP
#define STAN_MATH_PRIM_MAT_PROB_BERNOULLI_LOGIT_GLM_LPMF_HPP


/*
Fix includes
*/
#include <stan/math/prim/scal/meta/is_constant_struct.hpp>
#include <stan/math/prim/scal/meta/partials_return_type.hpp>
#include <stan/math/prim/scal/meta/operands_and_partials.hpp>
#include <stan/math/prim/scal/err/check_consistent_sizes.hpp>
#include <stan/math/prim/scal/err/check_bounded.hpp>
#include <stan/math/prim/scal/err/check_finite.hpp>
#include <stan/math/prim/scal/err/check_not_nan.hpp>
#include <stan/math/prim/scal/fun/constants.hpp>
#include <stan/math/prim/scal/fun/inv_logit.hpp>
#include <stan/math/prim/scal/fun/log1m.hpp>
#include <stan/math/prim/scal/fun/value_of.hpp>
#include <stan/math/prim/scal/meta/include_summand.hpp>
#include <stan/math/prim/scal/meta/scalar_seq_view.hpp>
#include <boost/random/bernoulli_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <cmath>
#include <string>

using Eigen::Dynamic;
using Eigen::Matrix;

namespace stan {
  namespace math {

    /**
     * Returns the log PMF of the Generalized Linear Model (GLM) with Bernoulli distribution
     * and logit link function. If containers are supplied, returns the log sum of the probabilities.
     *
     * @tparam T_n type of integer parameter
     * @tparam T_x type of the data matrix (design matrix); this should be an Eigen::Matrix type
     * @tparam T_beta type of the weight vector
     * @tparam T_alpha type of the bias vector
     * @param n integer parameter
     * @param x design matrix
     * @param beta weight vector
     * @param alpha bias vector (in logits)
     * @return log probability or log sum of probabilities
     * @throw std::domain_error if theta is infinite.
     * @throw std::invalid_argument if container sizes mismatch.
     */
	

	// The overloaded method addtorow is a bit of an ugly hack to satisfy the type checker. Hopefully, we can come up with something better.
	void addtorow(int n, internal::empty_broadcast_array<double> x, Matrix<double, 1,Dynamic > y)
	{
		std::cout << "bad";
	}
	void addtorow(int n, Matrix<double, Dynamic, Dynamic> x, Matrix<double, 1, Dynamic> y)
	{
		x.row(n) += y;
		std::cout << "good";
	}
	 
    template <bool propto, typename T_n, typename T_x, typename T_beta, typename T_alpha>
    typename return_type<T_x, T_beta, T_alpha>::type
    bernoulli_logit_glm_lpmf(const T_n& n, T_x& x, const T_beta& beta, const T_alpha& alpha) {
      static const std::string function = "bernoulli_logit_glm_lpmf";
      typedef typename stan::partials_return_type<T_n, T_x, T_beta, T_alpha>::type
        T_partials_return;

      using stan::is_constant_struct;
      using std::exp;

      if (!(stan::length(n) && stan::length(x) && stan::length(beta) && stan::length(alpha))) 
        return 0.0;

      T_partials_return logp(0.0);

      check_bounded(function, "n", n, 0, 1);
      check_not_nan(function, "Data matrix", x); 
      check_not_nan(function, "Weight vector", beta); 
      check_not_nan(function, "Bias vector", alpha);
      check_consistent_sizes(function,
                             "Random variable", n,
                             "Bias vector", alpha); 
      //check_consistent_sizes(function,
      //                       "Random variable", n,
      //                       "Data matrix", x); // would this work?
	  //			!!! add one more check to see that x is compatible with beta

      if (!include_summand<propto, T_x, T_beta, T_alpha>::value)
        return 0.0;

      scalar_seq_view<T_n> n_vec(n);
      scalar_seq_view<T_beta> beta_vec(beta);
      scalar_seq_view<T_alpha> alpha_vec(alpha);
      const size_t N = max_size(n, alpha); // do we want to check anything about x here? and in the next line?
	  const size_t M = beta.size();
      operands_and_partials<T_x, T_beta, T_alpha> ops_partials(x, beta, alpha);
	  Matrix<double, Dynamic, 1> beta_dbl(M,1);
	  Matrix<double, Dynamic, Dynamic> x_dbl(N,M);
	  for (size_t i = 0; i < M; i++) {  
		beta_dbl[i] = value_of(beta_vec[i]);
		for (size_t j = 0; j < N; j++) {
			x_dbl(j,i) = value_of(x(j,i));
		}
	  }
      for (size_t n = 0; n < N; n++) { // is there a point in vectorising this loop?
        const int n_int = value_of(n_vec[n]);
        const T_partials_return theta_dbl = (x_dbl.row(n)* beta_dbl)[0] + value_of(alpha_vec[n]); 


        const int sign = 2 * n_int - 1;
        const T_partials_return ntheta = sign * theta_dbl;
        const T_partials_return exp_m_ntheta = exp(-ntheta);

        // Handle extreme values gracefully using Taylor approximations.
        static const double cutoff = 20.0;
        if (ntheta > cutoff)
          logp -= exp_m_ntheta;
        else if (ntheta < -cutoff)
          logp += ntheta;
        else
          logp -= log1p(exp_m_ntheta);
		
		const bool constant_x = is_constant_struct<T_x>::value;
		const bool constant_beta = is_constant_struct<T_beta>::value;
		const bool constant_alpha = is_constant_struct<T_alpha>::value;
        if ( !(constant_x && constant_beta && constant_alpha)) { 
          static const double cutoff = 20.0; // do we really need this line?
		  T_partials_return theta_derivative;
          if (ntheta > cutoff)
            theta_derivative = - exp_m_ntheta;
          else if (ntheta < -cutoff)
            theta_derivative = sign;
          else
            theta_derivative = sign * exp_m_ntheta 
              / (exp_m_ntheta + 1);
          if ( !constant_beta){
            for (size_t m = 0; m < M; m++) // vectorise this loop?
			{
				ops_partials.edge2_.partials_[m] += theta_derivative * x_dbl(n,m);
			}		
		  }
		  if ( !constant_x){
			addtorow(n, ops_partials.edge1_.partials_, theta_derivative * beta_dbl.transpose());
		  }
		  if ( !constant_alpha){
            ops_partials.edge3_.partials_[n] += theta_derivative;	
          }			
        }
      }
      return ops_partials.build(logp);
    }

    template <typename T_n,
              typename T_x,
			  typename T_beta,
			  typename T_alpha>
    inline
    typename return_type<T_x, T_beta, T_alpha>::type
    bernoulli_logit_glm_lpmf(const T_n& n,
	                         const T_x& x,
							 const T_beta& beta,
							 const T_alpha& alpha) {
      return bernoulli_logit_glm_lpmf<false>(n, x, beta, alpha);
    }

  }
}
#endif
