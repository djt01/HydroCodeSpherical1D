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
 * @file EOS.hpp
 *
 * @brief Equation of state.
 *
 * @author Bert Vandenbroucke (bv7@st-andrews.ac.uk)
 */
#ifndef EOS_HPP
#define EOS_HPP

#if EOS == EOS_BONDI

// The Bondi specific EOS stuff is in the file Bondi.hpp
#include "Bondi.hpp"		//Numerical integration of source luminosity
//#include "BondiMC.hpp"		//TD Monte Carlo Simulation for rion

#else // EOS == EOS_BONDI

/**
 * @brief Set the initial value for the pressure of the given cell.
 *
 * For an ideal gas, we do not overwrite the value in the initial condition, and
 * this method does nothing.
 *
 * @param cell Cell.
 */
#if EOS == EOS_IDEAL
#define initial_pressure(cell)
#elif EOS == EOS_ISOTHERMAL
#define initial_pressure(cell) cell._P = ISOTHERMAL_C_SQUARED * cell._rho
#endif

/**
 * @brief Initialize ionisation variables.
 *
 * Not used for an ideal or isothermal equation of state.
 */
#define ionisation_initialize()

/**
 * @brief Code to determine the neutral fraction of the cells.
 *
 * do_ionisation uses numerical integration of source luminosity to establish
 * rion
 *
 * do_ionisation_MC uses time dependant Monte Carlo code to establish rion
 *
 * Not used for an ideal or isothermal equation of state.
 */
#define do_ionisation()
//#define do_ionisation_MC()

/**
 * @brief Code to handle the mass flux into the inner mask.
 *
 * Not used for an ideal or isothermal equation of state.
 *
 * @param mflux Mass flux into the inner mask (in internal units of M).
 */
#define flux_into_inner_mask(mflux)

/**
 * @brief Conversion function called during the primitive variable conversion
 * for the given cell.
 *
 * @param cell Cell.
 */
#if EOS == EOS_IDEAL
#define update_pressure(cell)                                                  \
  cell._P = (GAMMA - 1.) *                                                     \
            (cell._E / cell._V - 0.5 * cell._rho * cell._u * cell._u);
#elif EOS == EOS_ISOTHERMAL
#define update_pressure(cell) cell._P = ISOTHERMAL_C_SQUARED * cell._rho;
#endif

#endif // EOS == EOS_BONDI

#endif // EOS_HPP
