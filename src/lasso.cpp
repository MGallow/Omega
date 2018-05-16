// Matt Galloway

#include <RcppArmadillo.h>
#include <Rcpp.h>
#include "soft.h"

using namespace Rcpp;





//' @title Lasso Regression (c++)
//' 
//' @description Computes the coefficient estimates for lasso-penalized linear regression.
//' 
//' @details For details on the implementation of 'GLASSOO', see the vignette
//' \url{https://mgallow.github.io/GLASSOO/}.
//'
//' @param XX matrix
//' @param XY matrix
//' @param initB initialization for beta regression coefficients.
//' @param ind optional matrix specifying which coefficients will be penalized.
//' @param lam tuning parameter for lasso regularization term. Defaults to 'lam = 0.1'
//' @param crit criterion for convergence. Criterion \code{loss} will loop until the change in the objective after an iteration over the parameter set is less than \code{tol}. Criterion \code{sum} will loop until the sum change in the estimate after an interation over the parameter set is less than \code{tol} times tolerance multiple. Similary, criterion \code{max} will loop until the maximum change is less than \code{tol} times tolerance multiple. Defaults to \code{loss}.
//' @param tol tolerance for algorithm convergence. Defaults to 1e-4
//' @param maxit maximum iterations. Defaults to 1e4
//' 
//' @return returns list of returns which includes:
//' \item{Iterations}{number of iterations.}
//' \item{Coefficients}{estimated regression coefficients.}
//' \item{H}{update H matrix.}
//' 
//' @references
//' \itemize{
//' \item 
//' For more information on the coordinate descent algorithm, see: \cr
//' Friedman, Jerome, et al. "Pathwise coordinate optimization." \emph{The Annals of Applied Statistics} 1.2 (2007): 302-332.\cr
//' \url{https://arxiv.org/pdf/0708.1485.pdf}
//' }
//' 
//' @author Matt Galloway \email{gall0441@@umn.edu}
//' 
//' @keywords internal
//'
// [[Rcpp::export]]
List lassoc(const arma::mat &XX, const arma::mat &XY, const arma::mat &initB, const arma::mat &ind, const double lam = 0.1, std::string crit = "loss", const double tol = 1e-4, const double maxit = 1e4){

  // allocate memory
  int P = XX.n_cols, R = XY.n_cols, iter;
  double loss, loss2, temp;
  bool criterion = true;
  arma::mat B, B2, H, H2, maxes;
  H = H2 = arma::zeros<arma::mat>(P, R);
  maxes = arma::abs(arma::max(XY, 0));
  iter = loss = temp = 0;
  B2 = initB;
  
  // loop until convergence
  while (criterion && (iter < maxit)){

    // update values
    iter++;
    B = B2;
    loss = loss2;
    
    // loop over all entries of beta
    for (int r = 0; r < R; r++){
      for (int p = 0; p < P; p++){
        if (lam > maxes(r)){
          
          // set each entry in column r equal to zero if lam > max(XY)
          B2(p, r) = 0;
          
        } else {
        
          // otherwise update betas by soft thresholding
          B2(p, r) = softc(XY(p, r) - H2(p, r), lam*ind(p, r))/XX(p, p);
          
        }
        
        // if updated beta is different, update H matrix
        if (B2(p, r) != B(p, r)){
          
          // update each element in column r of H, except p
          temp = 0;
          H = H2;
          for (int p_ = 0; p_ < P; p_++){
            if (p_ != p){
              
              // update all rows except row p
              H2(p_, r) -= XX(p_, p)*(B(p, r) - B2(p, r));
              
              // create temporary sum used in loss function, if necessary
              if (crit == "loss"){
               temp += B2(p_, r)*XX(p_, p);
               
              }
            }
          }
          
          // update loss, if necessary
          if (crit == "loss"){
           loss2 += (B(p, r) - B2(p, r))*(XY(p, r) - temp) + (std::pow(B2(p, r), 2) - std::pow(B(p, r), 2))*XX(p, p)/2 + lam*ind(p,r)*(std::abs(B2(p, r)) - std::abs(B(p, r)));
            
          }
        }
      }
    }
    
    // stopping criterion
    if (crit == "loss"){
      
      // compute loss improvement
      criterion = (std::abs(loss2 - loss) > tol);
      
    } else if (crit == "avg") {
      
      // compute estimate avg change
      criterion = (arma::accu(arma::abs(B2 - B)) > tol);
      
    } else {
      
      // compute estimate max change
      criterion = (arma::abs(B2 - B).max() > tol);
      
    }

    // R_CheckUserInterrupt
    if (iter % 1000 == 0){
      R_CheckUserInterrupt();
    }
  }

  return List::create(Named("Iterations") = iter,
                      Named("Coefficients") = B2,
                      Named("H") = H2);

}

