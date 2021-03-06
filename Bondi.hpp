/*******************************************************************************
 * This file is part of HydroCodeSpherical1D
 * Copyright (C) 2017 Bert Vandenbroucke (bert.vandenbroucke@gmail.com)
 *
 * HydroCodeSpherical1D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HydroCodeSpherical1D is distributed in the hope that it will be useful,
 * but WITOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with HydroCodeSpherical1D. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

/**
 * @file Bondi.hpp
 *
 * @brief All Bondi problem specific code.
 *
 * @author Bert Vandenbroucke (bv7@st-andrews.ac.uk)
 */
#ifndef BONDI_HPP
#define BONDI_HPP

#include "Cell.hpp"           // Cell class
#include "Bank.hpp"           // Bank class
#include "LambertW.hpp"       // Lambert W function implementation
#include "SafeParameters.hpp" // Safe way to include Parameters.hpp

#include <cmath>

/*! @brief Bondi density: density at 
UNIT_TIME_IN_SI: 1.02197e+09
UNIT_DENSITY_IN_SI: 1.43461e-08
UNIT_VELOCITY_IN_SI: 11742
UNIT_PRESSURE_IN_SI: 1.97797
Newton G: 6.67408e-11 m^3 kg^-1 s^-2the neutral Bondi radius (in internal units
 *  of M L^-3). */
#define BONDI_DENSITY (BONDI_DENSITY_IN_SI / UNIT_DENSITY_IN_SI)

/*! @brief Neutral Bondi radius (in internal units of L). */
#define RBONDI (0.5 * G_INTERNAL * MASS_POINT_MASS / ISOTHERMAL_C_SQUARED)

// equation of state functionality for EOS_BONDI
#if EOS == EOS_BONDI

/**
 * @brief Initialize the log file used to log the ionisation radius as a
 * function of time.
 *
 * Only done if the ionisation mode is IONISATION_MODE_SELF_CONSISTENT.
 */
#if IONISATION_MODE == IONISATION_MODE_SELF_CONSISTENT
#define initialize_bondi_rfile()                                               \
  double rion_old = 0.;                                                        \
                                                                               \
  std::ofstream bondi_rfile("ionisation_radius.dat");
#elif IONISATION_MODE == IONISATION_MODE_MONTE_CARLO_TRANSFER
#define initialize_bondi_rfile()                                               \
  double rion_old = 0.;                                                        \
                                                                               \
  std::ofstream bondi_rfile("ionisation_radius.dat");
#elif IONISATION_MODE == IONISATION_MODE_CONSTANT
#define initialize_bondi_rfile()
#endif

/**
 * @brief Initialize ionisation variables.
 *
 * We need to initialize the smooth transition (if LINEAR_TRANSITION was
 * selected when configuring the code), and the ionising luminosity.
 * We also open the log file in which we will write the ionisation radius as a
 * function of time.
 */
#define ionisation_initialize()                                                \
  initialize_bondi_rfile();                                                    \
                                                                               \
  const double bondi_S =                                                       \
      (transition_width > 0.) ? 3. / (2. * transition_width) : 0.;             \
  const double bondi_A = -16. * bondi_S * bondi_S * bondi_S / 27.;             \
                                                                               \
  /* correction factor to grow the central mass with the accreted material     \
     currently disabled */                                                     \
  /*const double bondi_rmin = RMIN - CELLSIZE;                                 \
  const double bondi_volume_correction_factor = 4. * M_PI / 3. / CELLSIZE *    \
    (RMIN * RMIN * RMIN - bondi_rmin * bondi_rmin * bondi_rmin);*/             \
  const double bondi_volume_correction_factor = 0.;                            \
  std::cout << "Bondi volume correction factor: "                              \
            << bondi_volume_correction_factor << std::endl;                    \
                                                                               \
  /* set the Q value to the value that is needed to ionise out until the       \
     requested ionisation radius */                                            \
  _Pragma("omp parallel for") for (uint_fast32_t i = 1; i < ncell + 1; ++i) {  \
    const double rmin = cells[i]._lowlim;                                      \
    const double rmax = cells[i]._uplim;                                       \
    const double Vshell = (rmax * rmax * rmax - rmin * rmin * rmin) / 3.;      \
    const double Cshell = Vshell * cells[i]._rho * cells[i]._rho;              \
    cells[i]._nfac = Cshell;                                                   \
  }                                                                            \
  double const_bondi_Q = 0.;                                                   \
  for (uint_fast32_t i = 1; i < ncell + 1; ++i) {                              \
    const double rmin = cells[i]._lowlim;                                      \
    const double rmax = cells[i]._uplim;                                       \
    if (rmax < INITIAL_IONISATION_RADIUS) {                                    \
      const_bondi_Q += cells[i]._nfac;                                         \
    } else if (rmin < INITIAL_IONISATION_RADIUS) {                             \
      const double ifac =                                                      \
          (INITIAL_IONISATION_RADIUS * INITIAL_IONISATION_RADIUS *             \
               INITIAL_IONISATION_RADIUS -                                     \
           rmin * rmin * rmin) /                                               \
          (rmax * rmax * rmax - rmin * rmin * rmin);                           \
      const double Cshell = cells[i]._nfac * ifac;                             \
      const_bondi_Q += Cshell;                                                 \
    }                                                                          \
  }                                                                            \
  std::cout << "Bondi Q: " << const_bondi_Q << std::endl;                      \
  /* current value of the central mass (only used to increase the luminosity   \
     over time). Currently not really used. */                                 \
  double central_mass = MASS_POINT_MASS;

/**
 * @brief Set the initial value for the pressure of the given cell.
 *
 * We simply use the isothermal equation of state for this, as our scheme does
 * not use the initial pressure anyway (it's only used to initialise the total
 * energy, which is ignored in our integration scheme).
 *
 * @param cell Cell.
 */
#define initial_pressure(cell) cell._P = ISOTHERMAL_C_SQUARED * cell._rho

/**
 * @brief Conversion function called during the primitive variable conversion
 * for the given cell.
 *
 * This is the special equation of state that increase the pressure for ionised
 * gas.
 *
 * @param cell Cell.
 */
#define update_pressure(cell)                                                  \
  const double nfac = cells[i]._nfac;                                          \
  const double ifac = 1. - nfac;                                               \
  cells[i]._P = ISOTHERMAL_C_SQUARED * cells[i]._rho *                         \
                (bondi_pressure_contrast * ifac + nfac);

/**is:starred 
 * @brief Compute the ionisation radius by numerically integrating the density
 * squared until the desired target luminosity is reached.
 *
 * This is only done if IONISATION_MODE_SELF_CONSISTENT is chosen during
 * configuration.
 */
#if IONISATION_MODE == IONISATION_MODE_SELF_CONSISTENT
#define get_ionisation_radius()                                                \
  /* first loop */                                                             \
  _Pragma("omp parallel for") for (uint_fast32_t i = 1; i < ncell + 1; ++i) {  \
    const double rmin = cells[i]._lowlim;                                      \
    const double rmax = cells[i]._uplim;                                       \
    const double Vshell = (rmax * rmax * rmax - rmin * rmin * rmin) / 3.;      \
    const double Cshell = Vshell * cells[i]._rho * cells[i]._rho;              \
    cells[i]._nfac = Cshell;                                                   \
  }                                                                            \
                                                                               \
  /* second loop */                                                            \
  /* note that the get_bondi_Q_factor part defaults to 1 for now */            \
  double Cion =                                                                \
      const_bondi_Q * get_bondi_Q_factor(central_mass / MASS_POINT_MASS);      \
  double rion = 0.;                                                            \
  for (uint_fast32_t i = 1; i < ncell + 1; ++i) {                              \
    if (Cion > 0.) {                                                           \
      const double rmin = cells[i]._lowlim;                                    \
      const double rmax = cells[i]._uplim;                                     \
      const double ifac = std::min(1., Cion / cells[i]._nfac);                 \
      if (ifac < 1.) {                                                         \
        if (ifac > 0.) {                                                       \
          const double nfac = 1. - ifac;                                       \
          rion = std::cbrt(ifac * rmax * rmax * rmax +                         \
                           nfac * rmin * rmin * rmin);                         \
        }                                                                      \
      } else {                                                                 \
        rion = rmax;                                                           \
      }                                                                        \
      /* subtract this shell's ionisation budget from the total */             \
      Cion -= ifac * cells[i]._nfac;                                           \
    }                                                                          \
  }                                                                            \
  /* check if we need to write to the file */                                  \
  if (std::abs(rion - rion_old) > 1.e-4 * std::abs(rion + rion_old)) {         \
    const double curtime =                                                     \
        current_integer_time * time_conversion_factor * UNIT_TIME_IN_SI;       \
    const double ionrad = rion * UNIT_LENGTH_IN_SI;                            \
    Cion = const_bondi_Q * get_bondi_Q_factor(central_mass / MASS_POINT_MASS); \
    bondi_rfile.write(reinterpret_cast<const char *>(&curtime),                \
                      sizeof(double));                                         \
    bondi_rfile.write(reinterpret_cast<const char *>(&ionrad),                 \
                      sizeof(double));                                         \
    bondi_rfile.write(reinterpret_cast<const char *>(&Cion), sizeof(double));  \
    bondi_rfile.flush();                                                       \
    rion_old = rion;                                                           \
  }
#elif IONISATION_MODE == IONISATION_MODE_CONSTANT
#define get_ionisation_radius() const double rion = INITIAL_IONISATION_RADIUS;
#elif IONISATION_MODE == IONISATION_MODE_MONTE_CARLO_TRANSFER
#define get_ionisation_radius()                                                \
  /*const double rmin=cells[0]._lowlim*UNIT_LENGTH_IN_SI;*/                    \
  const double rmax=cells[ncell+1]._uplim;                                     \
  double Qion = 1.E47;                                                         \
  /*double timepassed=current_integer_time*time_conversion_factor;*/           \
  double taurem;                                                               \
  double rcurrent;                                                             \
  double lrem=SPEED_OF_LIGHT_IN_SI*cells[1]._dt*UNIT_TIME_IN_SI;               \
  double rion=0.0;							               \
  int nphoton=1000;                                                            \
  int cell;                                                                    \
  int sizebank=10000000;                                                       \
  int nbanki=P_Store[0]._currentnbank;                                         \
  int nbank=0;                                                                 \
  _Pragma("omp parallel for")                                                  \
  for (uint_fast32_t k = 1; k < ncell+1; ++k){                                 \
    cells[k]._jmean=0.0; cells[k]._length=0.0;}                                \
                                                                               \
  /* loop over packets stored in previous timestep */                          \
  for(uint_fast32_t j=0;j<nbanki;j++){                                         \
    taurem=P_Store[j]._currentTaurem;                                          \
    cell=P_Store[j]._currentCell;                                              \
    rcurrent=P_Store[j]._currentDistance;                                      \
    lrem=SPEED_OF_LIGHT_IN_SI*cells[0]._dt*UNIT_TIME_IN_SI;                    \
    if(cell>ncell){continue;}                                                  \
    if(rcurrent==0.0 && taurem==0.0){continue;}                                \
    if(rcurrent!=0.0){                                                         \
      ICT(cells,P_Store,cell,taurem,rcurrent,lrem,nbank);}                     \
    while(taurem > 0.0 && lrem>0.0){                                           \
      PROPAGATE(cells,P_Store,cell,taurem,lrem,nbank);}                        \
      }                                                                        \
                                                                               \
  /*loop over photons emitted from source in this timestep*/                   \
  for(uint_fast32_t j=0;j<nphoton;j++){                                        \
    taurem=-log(((double) rand()/RAND_MAX));                                   \
    cell=1;                                                                    \
    lrem=SPEED_OF_LIGHT_IN_SI*cells[1]._dt*UNIT_TIME_IN_SI;                    \
    while(taurem>0.0 && lrem >0.0){                                            \
      if (cell>ncell) {break;}                                                 \
        PROPAGATE(cells,P_Store,cell,taurem,lrem,nbank);}                      \
      }                                                                        \
                                                                               \
  /* calculate mean intensity in each cell based on total path length          \    
   * travelled through cell*/                                                  \
  _Pragma("omp parallel for")                                                  \
  for (uint_fast32_t k = 1; k < ncell+1; ++k){                                 \
    double vcell;                                                              \
    if ((cells[k]._lowlim*UNIT_LENGTH_IN_SI)==0.0){                            \
      vcell=4./3.*M_PI*std::pow(cells[k]._V*UNIT_LENGTH_IN_SI,3);}             \
    else{                                                                      \
      vcell=4.0/3.0*M_PI*(std::pow(cells[k]._uplim*UNIT_LENGTH_IN_SI,3)-       \
        std::pow(cells[k]._lowlim*UNIT_LENGTH_IN_SI,3));}                      \
    cells[k]._jmean=                                                           \
       (Qion*cells[k]._sigma*cells[k]._length)/(nphoton*vcell);}               \
                                                                               \
  /*update neutral fraction in each cell*/	                               \
  _Pragma("omp parallel for")                                                  \
    for (int k = 1; k < ncell+1; ++k){                                         \
      UPDATE_ION(cells,k,/*timepassed,*/cells[1]._dt*UNIT_TIME_IN_SI);         \
      cells[k]._last_jmean=cells[k]._jmean;}                                   \
                                                                               \
  /* finish simulation if number of photons that are being stored              \
   * exceeds size of bankelse set number of photons stored this step           \
   * as nbanki and reset nbank to 0 */                                         \
  if (nbank>sizebank){                                                         \
    break;}                                                                    \
                                                                               \
  /* shift packets stored this step to first three columns of bank to          \
   * be read from next step */                                                 \
  _Pragma("omp parallel for")                                                  \
  for(uint_fast32_t j=0;j<nbank;j++){                                          \
    P_Store[j]._currentTaurem=P_Store[j]._futureTaurem;                        \
    P_Store[j]._currentCell=P_Store[j]._futureCell;                            \
    P_Store[j]._currentDistance=P_Store[j]._futureDistance;                    \
    P_Store[j]._futureTaurem=0.0;                                              \
    P_Store[j]._futureDistance=0.0;                                            \
    P_Store[j]._futureCell=0;}			                               \
  P_Store[0]._currentnbank=nbank;                                              \
                                                                               \
  /* calculate rion */                                                         \
  for (uint_fast32_t k=1;k<ncell+2;k++){                                       \
    /*std::cout << "Calculating rion, cell" << k << std::endl;                 \
    */if (cells[k]._nfac_MC>=0.5){                                        \
      rion=cells[k]._lowlim;                                                   \
      break;                                                                   \
      if (k>ncell){rion=rmax;}}                                                \
    }                                                                          \
                                                                               \
  /* check if we need to write to the file */                                  \
  if (std::abs(rion - rion_old) > 1.e-4 * std::abs(rion + rion_old)) {         \
    const double curtime =                                                     \
        current_integer_time * time_conversion_factor * UNIT_TIME_IN_SI;       \
    const double ionrad = rion * UNIT_LENGTH_IN_SI;                            \
    double Cion = Qion;                                                        \
    bondi_rfile.write(reinterpret_cast<const char *>(&curtime),                \
                      sizeof(double));                                         \
    bondi_rfile.write(reinterpret_cast<const char *>(&ionrad),                 \
                      sizeof(double));                                         \
    bondi_rfile.write(reinterpret_cast<const char *>(&Cion), sizeof(double));  \
    bondi_rfile.flush();                                                       \
    rion_old = rion;                                                           \
  }
#endif

/**
 * @brief Code to determine the neutral fraction of the cells.
 *
 * This code does three loops over all cells (of which two are done in
 * parallel):
 *  - a first loop to compute the integrated density squared for each cell
 *  - a second loop that figures out when the summed integrated density squared
 *    equals the target luminosity and computes the ionisation radius
 *  - a final loop that sets the neutral fraction once the ionisation radius is
 *    known
 * The first two loops are done in get_ionisation_radius, and are not done if
 * we keep the ionisation radius fixed.
 */
#define do_ionisation()                                                        \
  /* compute the ionisation radius rion */                                     \
  get_ionisation_radius();                                                     \
                                                                               \
  /* third loop */                                                             \
  const double rion_min = rion - 0.5 * transition_width;                       \
  const double rion_max = rion + 0.5 * transition_width;                       \
  _Pragma("omp parallel for") for (uint_fast32_t i = 1; i < ncell + 1; ++i) {  \
    cells[i]._nfac =                                                           \
        get_neutral_fraction(cells[i]._lowlim, cells[i]._uplim, rion,          \
                             rion_min, rion_max, bondi_S, bondi_A);            \
  }

/**
 * @brief Code to handle the mass flux into the inner mask.
 *
 * We set bondi_volume_correction_factor to zero, so this code does not
 * currently do anything.
 *
 * @param mflux Mass flux into the inner mask.
 */
#define flux_into_inner_mask(mflux)                                            \
  central_mass -= mflux * bondi_volume_correction_factor;

#endif // EOS == EOS_BONDI

// boundary condition functionality
#if BOUNDARIES == BOUNDARIES_BONDI

/**
 * @brief Initialize variables used for the boundary conditions.
 *
 * We need to initialize the outer boundary variables.
 */
#define boundary_conditions_initialize()                                       \
  const double bondi_r_inv_high = RBONDI / cells[ncell + 1]._midpoint;         \
  const double bondi_density_high = bondi_density(bondi_r_inv_high);           \
  const double bondi_velocity_high = bondi_velocity(bondi_r_inv_high);         \
  const double bondi_pressure_high = bondi_pressure(bondi_r_inv_high);         \
                                                                               \
  const double bondi_rmax_inv =                                                \
      RBONDI / (cells[ncell + 1]._midpoint + CELLSIZE);                        \
  const double bondi_density_max = bondi_density(bondi_rmax_inv);              \
  const double bondi_velocity_max = bondi_velocity(bondi_rmax_inv);            \
  const double bondi_pressure_max = bondi_pressure(bondi_rmax_inv);

/**
 * @brief Apply boundary conditions after the primitive variable conversion.
 */
#define boundary_conditions_primitive_variables()                              \
  /* impose the Bondi solution at the boundaries */                            \
  /* lower boundary: outflow */                                                \
  cells[0]._rho = cells[1]._rho;                                               \
  cells[0]._u = cells[1]._u;                                                   \
  cells[0]._P = cells[1]._P;                                                   \
                                                                               \
  /* upper boundary: neutral Bondi solution */                                 \
  cells[ncell + 1]._rho = bondi_density_high;                                  \
  cells[ncell + 1]._u = bondi_velocity_high;                                   \
  cells[ncell + 1]._P = bondi_pressure_high;

/**
 * @brief Apply boundary conditions after the gradient computation.
 */
#define boundary_conditions_gradients()                                        \
  /* lower boundary: outflow */                                                \
  {                                                                            \
    cells[0]._grad_rho = cells[1]._grad_rho;                                   \
    cells[0]._grad_u = cells[1]._grad_u;                                       \
    cells[0]._grad_P = cells[1]._grad_P;                                       \
  }                                                                            \
                                                                               \
  /* upper boundary: compute gradient using the known expression outside rmax  \
   */                                                                          \
  {                                                                            \
    const double dx_inv = 1. / CELLSIZE;                                       \
    const double half_dx = HALF_CELLSIZE;                                      \
                                                                               \
    const double gradrho = (bondi_density_max - cells[ncell]._rho) * dx_inv;   \
    const double rhomax = std::max(cells[ncell]._rho, bondi_density_max);      \
    const double rhomin = std::min(cells[ncell]._rho, bondi_density_max);      \
    const double rho_ext_plu = half_dx * gradrho;                              \
    const double rho_ext_min = -half_dx * gradrho;                             \
    const double rhoextmax = std::max(rho_ext_min, rho_ext_plu);               \
    const double rhoextmin = std::min(rho_ext_min, rho_ext_plu);               \
    const double alpha_rho =                                                   \
        (gradrho != 0.)                                                        \
            ? std::min(                                                        \
                  1., 0.5 * std::min(                                          \
                                (rhomax - cells[ncell + 1]._rho) / rhoextmax,  \
                                (rhomin - cells[ncell + 1]._rho) / rhoextmin)) \
            : 1.;                                                              \
    cells[ncell + 1]._grad_rho = alpha_rho * gradrho;                          \
                                                                               \
    const double gradu = (bondi_velocity_max - cells[ncell]._u) * dx_inv;      \
    const double umax = std::max(cells[ncell]._u, bondi_velocity_max);         \
    const double umin = std::min(cells[ncell]._u, bondi_velocity_max);         \
    const double u_ext_plu = half_dx * gradu;                                  \
    const double u_ext_min = -half_dx * gradu;                                 \
    const double uextmax = std::max(u_ext_min, u_ext_plu);                     \
    const double uextmin = std::min(u_ext_min, u_ext_plu);                     \
    const double alpha_u =                                                     \
        (gradu != 0.)                                                          \
            ? std::min(1.,                                                     \
                       0.5 * std::min((umax - cells[ncell + 1]._u) / uextmax,  \
                                      (umin - cells[ncell + 1]._u) / uextmin)) \
            : 1.;                                                              \
    cells[ncell + 1]._grad_u = alpha_u * gradu;                                \
                                                                               \
    const double gradP = (bondi_pressure_max - cells[ncell]._P) * dx_inv;      \
    const double Pmax = std::max(cells[ncell]._P, bondi_pressure_max);         \
    const double Pmin = std::min(cells[ncell]._P, bondi_pressure_max);         \
    const double P_ext_plu = half_dx * gradP;                                  \
    const double P_ext_min = -half_dx * gradP;                                 \
    const double Pextmax = std::max(P_ext_min, P_ext_plu);                     \
    const double Pextmin = std::min(P_ext_min, P_ext_plu);                     \
    const double alpha_P =                                                     \
        (gradP != 0.)                                                          \
            ? std::min(1.,                                                     \
                       0.5 * std::min((Pmax - cells[ncell + 1]._P) / Pextmax,  \
                                      (Pmin - cells[ncell + 1]._P) / Pextmin)) \
            : 1.;                                                              \
    cells[ncell + 1]._grad_P = alpha_P * gradP;                                \
  }

#endif // BOUNDARIES == BOUNDARIES_BONDI

/**
 * @brief Squared neutral Bondi velocity divided by the sound speed squared.
 *
 * @param rinv Inverse radius (in units of RBONDI^-1).
 * @return Bondi velocity squared divided by the sound speed squared.
 */
double u2_over_cs2(double rinv) {
  const double lambertarg = -std::exp(3. + 4. * (std::log(rinv) - rinv));
  if (rinv < 1.) {
    return -LambertW::lambert_w(lambertarg, 0);
  } else {
    return -LambertW::lambert_w(lambertarg, -1);
  }
}

/**
 * @brief Get the value of the neutral Bondi density at the given inverse
 * radius.
 *
 * @param rinv Inverse radius (in units of RBONDI^-1).
 * @return Value of the density (in internal units of M L^-3).
 */
double bondi_density(double rinv) {
  // we need to manually disable the density very close to r = 0 to prevent
  // errors in the Lambert W function
  if (rinv < 150.) {
    return BONDI_DENSITY * std::exp(-0.5 * u2_over_cs2(rinv) + 2. * rinv - 1.5);
  } else {
    return 0.;
  }
}

/**
 * @brief Get the value of the neutral Bondi velocity at the given inverse
 * radius.
 *
 * @param rinv Inverse radius (in internal units of L^-1).
 * @return Value of the fluid velocity (in internal units of L T^-1).
 */
double bondi_velocity(double rinv) {
  return -std::sqrt(ISOTHERMAL_C_SQUARED * u2_over_cs2(rinv));
}

/**
 * @brief Get the value of the neutral Bondi pressure at the given inverse
 * radius.
 *
 * @param rinv Inverse radius (in internal units of L^-1).
 * @return Value of the pressure (in internal units of M L^-1 T^-2).
 */
double bondi_pressure(double rinv) {
  return ISOTHERMAL_C_SQUARED * bondi_density(rinv);
}

/**
 * @brief Get the increase in luminosity corresponding to the given increase in
 * central mass.
 *
 * This expression was fitted to the values in Keto (2003).
 *
 * @param M Central mass (in units of the initial central mass).
 * @return Luminosity increase for the given mass increase.
 */
inline static double get_bondi_Q_factor(const double M) {
  return 7.96185873 * (std::pow(M, 2.47692987) - 1.) + 1.;
}

/**
 * @brief Get the indefinite integral of the smooth neutral fraction polynomial.
 *
 * For reference: the polynomial has general shape
 * \f[
 *   f(r) = A (r - R_i)^3 + B (r - R_i)^2 + C (r - R_i) + D,
 * \f]
 * with \f$A,B,C,D\f$ constant parameters, and \f$R_i\f$ the ionisation radius.
 * We apply the following conditions to find the unknown parameters:
 *  - at the ionisation radius, the value of the polynomial should be 0.5:
 *    \f[
 *      f(R_i) = \frac{1}{2}.
 *    \f]
 *  - at the ionisation radius, the value of the first derivative should be
 *    \f$S\f$, the maximal allowed slope for the polynomial:
 *    \f[
 *      f'(R_i) = S.
 *    \f]
 *  - at the points \f$r_l = R_i - \frac{1}{2}W\f$ and \f$r_u = R_i +
 *    \frac{1}{2}W\f$, the values of the polynomial should be 0 and 1:
 *    \f[
 *      f(R_i - \frac{1}{2}W) = 0,
 *    \f]
 *    \f[
 *      f(R_i + \frac{1}{2}W) = 1.
 *    \f]
 *  - at \f$r_l\f$ and \f$r_u\f$, the first derivative should be zero to get a
 *    smooth connection between the polynomial and the constant regions:
 *    \f[
 *      f'(R_i - \frac{1}{2}W) = 0,
 *    \f]
 *    \f[
 *      f'(R_i + \frac{1}{2}W) = 0.
 *    \f]
 * We find the following simplified expression for the polynomial:
 * \f[
 *   f(r) = -\frac{16}{27}S^3 (r - R_i)^3 + S (r - R_i) + \frac{1}{2},
 * \f]
 * with \f$W = \frac{3}{2S}\f$.
 *
 * The function below is the indefinite integral
 * \f[
 *   60 \int f(r) r^2 {\rm{}d}r
 * \f]
 * (the prefactor 60 is used to save on a factor 3 in the total neutral fraction
 * computation and a division by 20 in the definite integral computation below).
 *
 * @param A Smooth transition parameter A (in internal units of L^-3).
 * @param S Smooth transition parameter S (steepest allowed slope in the
 * transition; in internal units of L^-1).
 * @param rion Ionisation radius (in internal units of L).
 * @param r Point where we want to evaluate the indefinite integral (in internal
 * units of L).
 * @return Indefinite integral (in integral units of L; times 20).
 */
inline static double neutral_fraction_integral_function(const double A,
                                                        const double S,
                                                        const double rion,
                                                        const double r) {
  const double r2 = r * r;
  const double r3 = r * r2;
  const double rion2 = rion * rion;
  const double rion3 = rion * rion2;
  return r3 * (10. * r3 * A - 36. * r2 * A * rion + 45. * r * A * rion2 +
               15. * r * S - 20. * A * rion3 - 20. * rion * S + 10.);
}

/**
 * @brief Get the neutral fraction within the given interval using an exact
 * integration of the smooth transition expression.
 *
 * @param A Smooth transition parameter A (in internal units of L^-3).
 * @param S Smooth transition parameter S (steepest allowed slope in the
 * transition; in internal units of L^-1).
 * @param rion Ionisation radius (in internal units of L).
 * @param rmin Lower limit of the integration interval (in internal units of L).
 * @param rmax Upper limit of the integration interval (in internal units of L).
 * @return Integrated neutral fraction within the interval (in internal units of
 * L).
 */
inline static double
get_neutral_fraction_integral(const double A, const double S, const double rion,
                              const double rmin, const double rmax) {
  return (neutral_fraction_integral_function(A, S, rion, rmax) -
          neutral_fraction_integral_function(A, S, rion, rmin)) /
         20.;
}

/**
 * @brief Get the neutral fraction for the cell with the given midpoint radius.
 *
 * @param rmin Radius of the lower wall of the cell (in internal units of L).
 * @param rmax Radius of the upper wall of the cell (in internal units of L).
 * @param rion Ionisation radius (in internal units of L).
 * @param rion_min Ionisation radius minus smooth transition width (in internal
 * units of L).
 * @param rion_max Ionisation radius plus smooth transition width (in internal
 * units of L).
 * @param A Smooth transition parameter A (in internal units of L^-3).
 * @param S Smooth transition parameter S (steepest allowed slope in the
 * transition; in internal units of L^-1).
 * @return Neutral fraction within the cell.
 */
inline static double get_neutral_fraction(const double rmin, const double rmax,
                                          const double rion,
                                          const double rion_min,
                                          const double rion_max, const double S,
                                          const double A) {
#if IONISATION_TRANSITION == IONISATION_TRANSITION_SMOOTH
  // Note: we assume rmax - rmin << rion_max - rion_min
  if (rmax < rion_min) {
    return 0.;
  } else if (rmin < rion_min && rmax >= rion_min) {
    return get_neutral_fraction_integral(A, S, rion, rion_min, rmax) /
           (rmax * rmax * rmax - rmin * rmin * rmin);
  } else if (rmin >= rion_min && rmax <= rion_max) {
    return get_neutral_fraction_integral(A, S, rion, rmin, rmax) /
           (rmax * rmax * rmax - rmin * rmin * rmin);
  } else if (rmin < rion_max && rmax > rion_max) {
    return (get_neutral_fraction_integral(A, S, rion, rmin, rion_max) +
            (rmax * rmax * rmax - rion_max * rion_max * rion_max)) /
           (rmax * rmax * rmax - rmin * rmin * rmin);
  } else {
    return 1.;
  }
#elif IONISATION_TRANSITION == IONISATION_TRANSITION_JUMP
  if (rmax < rion) {
    return 0;
  } else if (rmin < rion) {
    return (rmax * rmax * rmax - rion * rion * rion) /
           (rmax * rmax * rmax - rmin * rmin * rmin);
  } else {
    return 1.;
  }
#endif
}

// Bondi initial condition functionality
#if IC == IC_BONDI
/**
 * @brief Initialize the given cells.
 *
 * We start with a constant density and velocity everywhere, the value of which
 * corresponds to the inflow values.
 *
 * @param cells Cells to initialize.
 * @param ncell Number of cells.
 */
#define initialize(cells, ncell)                                               \
  _Pragma("omp parallel for") for (unsigned int i = 1; i < ncell + 1; ++i) {   \
    const double r_inv = RBONDI / RMAX;                                        \
    cells[i]._rho = bondi_density(r_inv);                                      \
    cells[i]._u = bondi_velocity(r_inv);                                       \
    cells[i]._P = ISOTHERMAL_C_SQUARED * cells[i]._rho;                        \
    const double r2 = cells[i]._midpoint * cells[i]._midpoint;                 \
    cells[i]._a = -G_INTERNAL * MASS_POINT_MASS / r2;                          \
    cells[i]._nfac = 0.;                                                       \
  }

#endif // IC == IC_BONDI

/**
 * @brief Store photon packets for which distance travelled exceeds that 
 * allowed in one timestep.
 *
 * @param cell Cell location of packet to be stored.
 * @param taurem Remaining optical depth packet has to travel.
 * @param radius Distance from current cell lower boundary that packet has 
 * travelled (in internal units of L).
 * @param stored Number of packets added to bank so far this timestep
 */

void BANK(Bank* P_Store,int& cell,double& taurem,double& radius,int& stored){
  P_Store[stored]._futureCell=cell;                                           
  P_Store[stored]._futureDistance=radius;                                     
  P_Store[stored]._futureTaurem=taurem;                                       
  stored++;}                                                                   
  
/**
 * @brief Propagates stored packets through to end of cell in which they were
 * stored (if they can reach end of cell in timestep without being absorbed).
 *
 * @param cell Cell location of packet.
 * @param taurem Remaining optical depth packet has to travel.
 * @param rcurrent Distance from current cell lower boundary that packet has 
 * travelled (in internal units of L).
 * @param lrem Distance packet can travel in this timestep (in internal units 
 * of L).
 * @param stored Number of packets added to bank so far this timestep
 */

void ICT(Cell* cells,Bank* P_Store,int& cell, double& taurem,double& rcurrent,
         double& lrem,int& stored){                                 
  double taucell=                                                             
              cells[cell]._sigma*(cells[cell]._V*UNIT_LENGTH_IN_SI-rcurrent)* 
              cells[cell]._rho*(UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)*      
              cells[cell]._nfac_MC;                                              
  if (taurem>taucell && lrem>(cells[cell]._V*UNIT_LENGTH_IN_SI-rcurrent)){    
    cells[cell]._length+=(cells[cell]._V*UNIT_LENGTH_IN_SI-rcurrent);         
    taurem-=taucell;                                                          
    lrem-=(cells[cell]._V*UNIT_LENGTH_IN_SI-rcurrent);                        
    cell++;                                                                   
    rcurrent=0.0;}                                                            
  else {                                                                      
    double taulength=                                                         
              taurem/(cells[cell]._rho*                                       
              (UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)*cells[cell]._nfac_MC*     
              cells[cell]._sigma);                                            
    if (taulength<=lrem){                                                     
    cells[cell]._length+=taulength;                                           
    taurem=0.0;}                                                              
    else{                                                                     
      cells[cell]._length+=lrem;                                              
      taurem-=(cells[cell]._rho*(UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)*     
               cells[cell]._nfac_MC*cells[cell]._sigma*lrem);                    
      rcurrent+=lrem;                                                       
      if (stored<=10000000){BANK(P_Store,cell,taurem,rcurrent,stored);}               
      else{stored++;}                                                         
      lrem=0.0;}                                                              
     }}                                                                       
  

/**
 * @brief Propagates packet through current cell. If packet is unable to  
 * reach end of  cell either absorbed (remaining optical depth to travel 
 * less than cell optical depth) and terminated, or banked (if cannot  
 * travel physical distance of cell in remaining timestep). 
 *
 * @param cell Cell location of packet.
 * @param taurem Remaining optical depth packet has to travel.
 * @param lrem Distance packet can travel in this timestep (in internal  
 * units of L).
 * @param stored Number of packets added to bank so far this timestep
 */

void PROPAGATE(Cell* cells,Bank* P_Store, int& cell,double& taurem,
               double& lrem,int& stored){
  double taucell=                                                              
              cells[cell]._sigma*cells[cell]._V*UNIT_LENGTH_IN_SI*             
              cells[cell]._rho*(UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)*       
              cells[cell]._nfac_MC;                                              
  if (taurem>taucell && lrem>cells[cell]._V*UNIT_LENGTH_IN_SI){                
    cells[cell]._length+=cells[cell]._V*UNIT_LENGTH_IN_SI;                     
    taurem-=taucell;                                                           
    lrem-=cells[cell]._V*UNIT_LENGTH_IN_SI;                                    
    cell++;}                                                                   
  else {                                                                       
    double taulength=                                                          
              taurem/(cells[cell]._rho*                                        
              (UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)*cells[cell]._nfac_MC*      
              cells[cell]._sigma);                                             
    if (taulength<=lrem){                                                      
    cells[cell]._length+=taulength;                                            
    taurem=0.0;}                                                              
    else{                                                                      
      cells[cell]._length+=lrem;                                               
      taurem-=(cells[cell]._rho*(UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)*      
               cells[cell]._nfac_MC*cells[cell]._sigma*lrem);                   
      if (stored<=10000000){BANK(P_Store,cell,taurem,lrem,stored);}                    
      else{stored++;}                                                          
      lrem=0.0;}                                                               
    }}                                                                         

/**
 * @brief updates neutral fraction in cell according to solution to 
 * differential equation df/dt=(1-f)*jmean-f^2*(ntot*alphaB) where f is the
 * ionised fraction of the cell.
 *
 * @param cell Cell location of packet.
 * @param timep t_0 : simulation time of previous step
 * @param delta Simulation time since t_0
 */

void UPDATE_ION(Cell* cells,int& cell,/*timep,*/double delta){ 
  double ConB=                                                                
              (cells[cell]._alphaB*(cells[cell]._rho*                         
              (UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)));                     
  /*double last_ConB=                                                         
              (cells[cell]._alphaB*(cells[cell]._last_rho*                    
              (UNIT_DENSITY_IN_SI/HYDROGEN_MASS_IN_SI)));*/                   
  if(cells[cell]._nfac_MC==1.0){                                                
    cells[cell]._ifrac=                                                       
              cells[cell]._ft0+cells[cell]._jmean*delta;/*+                   
              (cells[cell]._jmean-cells[cell]._last_jmean)*timep;*/           
    if(cells[cell]._ifrac>1.0){                                               
      cells[cell]._nfac_MC=1.0E-8;                                               
      cells[cell]._ifrac=1.0-cells[cell]._nfac_MC;}                              
    else{cells[cell]._nfac_MC=1.0-cells[cell]._ifrac;}                           
    cells[cell]._ft0=cells[cell]._ifrac;                                      
  }                                                                           
  else{                                                                       
    if(cells[cell]._jmean==0.0 or cells[cell]._nfac_MC==0.0){ 
      cells[cell]._ifrac=1./((1./cells[cell]._ft0)+ConB*delta);               
      cells[cell]._nfac_MC=1.0-cells[cell]._ifrac;                               
      cells[cell]._ft0=cells[cell]._ifrac;}                                   
    else{   
      double IoR=(cells[cell]._jmean)/ConB;                                   
      double root=std::sqrt(IoR*(IoR+4.));                                         
      /*double last_IoR=(cells[cell]._last_jmean)/last_ConB;                  
      double last_root=sqrt(last_IoR*(last_IoR+4.));*/                        
      double arg=(2*cells[cell]._ft0+IoR)/root;                               
      double arg2=0.5*ConB*root*delta;
      cells[cell]._ifrac=                                                     
              0.5*root*((arg+std::tanh(arg2))/(1.+arg*std::tanh(arg2)))-
              0.5*IoR;        
      cells[cell]._nfac_MC=1.0-cells[cell]._ifrac;                          
      cells[cell]._ft0=cells[cell]._ifrac;}                                   
  }}

#endif // BONDI_HPP
