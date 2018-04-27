# write the configuration command for a Bondi setup run

import get_cmake_command

bondi_options = {
"rmin_in_au": 10.,
"rmax_in_au": 100.,
"ncell": 2700,
"gamma": 1.001,
"maxtime_in_yr": 40.,
"number_of_snaps": 2000,
"ic": "IC_BONDI",
"eos": "EOS_BONDI",
"boundaries": "BOUNDARIES_BONDI",
"isothermal_temperature_in_k": 500.,
"potential": "POTENTIAL_POINT_MASS",
"g_internal": 1.,
"mass_point_mass_in_msol": 18.,
"bondi_density_in_si": 1.e-16,
"bondi_pressure_contrast": 32.,
"initial_ionisation_radius_in_au": 100.,
"unit_mass_in_si": 2.479e31,
"unit_length_in_si": 1.2e13,
"ionisation_mode": "IONISATION_MODE_CONSTANT",
"ionisation_transition": "IONISATION_TRANSITION_SMOOTH",
"ionisation_transition_width_in_au": 5.,
"courant_factor": 0.05,
"riemannsolver_type": "RIEMANNSOLVER_TYPE_HLLC"
}

print get_cmake_command.get_cmake_command(bondi_options)
