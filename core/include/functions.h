
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

namespace core
{

/** \brief 1D-Function defining a sloped plateau with two gaussian edges with 8 parameters
 *
 * \param xx[0] Indeopendent variable
 * \param par[0] x_0, left end position of the plateau-slope
 * \param par[1] x_1, right end position of the plateau-slope
 * \param par[2] y_0, height of plateau at x_0
 * \param par[3] y_1, height of plateau at x_1
 * \param par[4] sigma_l, sigma of the left gausian tail
 * \param par[5] sigma_r, sigma of the right gaussian tail
 * \param par[6] h_0, height of the left gaussian tail
 * \param par[7] h_1, height of the right gaussian tail
 */
double general_plateau_function(double* xx, double* par);

/** \brief A simplified, symmetric 1D-plateau function with gaussian edges, with 5 parameters.
 *
 * \param xx[0] Independent variable
 * \param par[0] x_0, left end of the plateau
 * \param par[1] x_1, right end of the plateau
 * \param par[2] y_plat, height of the plateau on Y axis
 * \param par[3] sigma, steepness of gaussian edges
 * \param par[4] h_gaus, height of the gaussian edges
 */
double symmetric_plateau_function(double* xx, double* par);

}

#endif
