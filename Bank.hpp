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
 * @file Bank.hpp
 *
 * @brief Single element of the photon packet storage bank for MC photoionisation.
 *
 * @author Daniel Tootill (dt34@st-andrews.ac.uk)
 */
#ifndef BANK_HPP
#define BANK_HPP

#include <cstdint>

/**
 * @brief Single element of the photon packet storage bank.
 */
class Bank {
public:
  // variables for stored packets to be used in current timestep

  /*! @brief Current grid cell. */
  int _currentCell;
  /*! @brief Remaining optical depth for packet to travel. */
  double _currentTaurem;
  /*! @brief Current physical position of packet in grid cell, in relation to 
   *  lower boundary (in internal units of L). */
  double _currentDistance;



  // variables for stored packets to be used in future timestep

  /*! @brief Current grid cell. */
  int _futureCell;
  /*! @brief Remaining optical depth for packet to travel. */
  double _futureTaurem;
  /*! @brief Current physical position of packet in grid cell, in relation to 
   *  lower boundary (in internal units of L). */
  double _futureDistance;
};

#endif // BANK_HPP
