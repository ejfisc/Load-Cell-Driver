/* ****************************************************************************/
/** Strain Guage Driver Library 

  @File Name
    strain_guage.h

  @Summary
    Driver Library for a strain guage, uses the hx711f adc driver

  @Description
    Defines functions that allow the user to interact with the strain guage
******************************************************************************/

#ifndef STRAIN_GUAGE_H
#define STRAIN_GUAGE_H

#include <inttypes.h>

// load cell specification
typedef struct {
    uint16_t capacity; // capacity in kg
    float VE; // excitation voltage V
    float RO; // rated output mV/V
    float offset; // offset used for taring
}StrainGauge;

/*
    @brief Strain Gauge Initialization

    @note Sets specifications used for calculating kgs

    @param[in] ve Excitation / Power Supply Voltage (usually 5V)

    @param[in] capacity Load Cell capacity (3kg, 10kg, 50kg, etc.)

    @param[in] ro Rated Output in mV/V
*/
void strain_gauge_init(float ve, uint16_t capacity, float ro);

/*
    @brief Function for reading kilogram measurement from strain gauge

    @note Calculates kilograms based on load cell specifications, line of best fit, and tare offset

    @ret Current kilogram measurement (float)
*/
float read_kgs(void);

/*
    @brief Function for reading pound measurement from strain gauge

    @note Calculates pounds based on kilogram measurement

    @ret Current pound measurement (float)
*/
float read_lbs(void);

/*
    @brief Function for reading an average measurement

    @note Uses the external read_sg flag that's set on a timer interrupt to call read_kgs()
	so that there are no timing issues, and no delays are used

    @param[in] times How many times to sample the strain gauge for the average

    @ret Average kg measurement
*/
float read_average(uint8_t times);

/*
    @brief Tare the strain gauge

    @note Calculates the average measurement and sets that as the offset to get the reading to 0.
*/
void strain_gauge_tare(void);

/*
    @brief Calibrate the strain gauge with known weights

    @note Takes an array of known weight values, and calculates a line of best fit equation using
	the known weights as y data and the measured averages per weight as x data. 

    @param[in] weight_cnt Number of known weights

    @param[in] known_weights Float array of known weight values

    @param[in] equation Pointer to a float array to populate with calibration factors
*/
void strain_gauge_calibrate(uint8_t weight_cnt, float * known_weights, float * equation);

/*
    @brief Calculate the line of best fit equation given x and y data points

    @param[in] weight_cnt Number of known weights

    @param[in] x X data points, should be measured averages

    @param[in] y Y data points, should be known weights

    @param[in] equation Pointer to a float array to populate with calibration factors
*/
void strain_gauge_calculate_equation(uint8_t weight_cnt, float * x, float * y, float * equation);

/*
    @brief Set line of best fit equation

    @note Sets calibration factors

    @param[in] m Slope for line of best fit equation

    @param[in] b Intercept for line of best fit equation
*/
void strain_gauge_set_equation(float m, float b);


#endif // STRAIN_GUAGE_H