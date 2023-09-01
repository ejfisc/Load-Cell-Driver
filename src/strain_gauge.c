/* ****************************************************************************/
/** Strain Guage Driver Library 

  @File Name
    strain_guage.c

  @Summary
    Driver Library for a strain guage, uses the hx711f adc driver

  @Description
    Implements functions that allow the user to interact with the strain guage
******************************************************************************/

#include "strain_gauge.h"
#include "hx711_adc.h"
#include <inttypes.h>
#include "nrf_delay.h" // nordic sdk specific delay

#define delay_ms(time) nrf_delay_ms(time) // macro to redirect to SDK specific delay function

//#define DEBUG_OUTPUT // comment this line to turn off prints

StrainGauge sg; // strain gauge instance

volatile bool taring = false; // taring flag used to indicate that the strain gauge is being tared
volatile bool calibrating = false; // calibrating flag used to indicate that the strain gauge is being calibrated 
extern bool read_sg; // read strain gauge flag, set on interrupt in main.c, used in read_average() to prevent timing issues

// line of best fit calibration factors
static float slope = 0;
static float intercept = 0;

/*
    @brief Strain Gauge Initialization

    @note Sets specifications used for calculating kgs

    @param[in] ve Excitation / Power Supply Voltage (usually 5V)

    @param[in] capacity Load Cell capacity (3kg, 10kg, 50kg, etc.)

    @param[in] ro Rated Output in mV/V
*/
void strain_gauge_init(float ve, uint16_t capacity, float ro) {
    sg.capacity = capacity;
    sg.VE = ve;
    sg.RO = ro;
    sg.offset = 0;
}

/*
    @brief Function for reading kilogram measurement from strain gauge

    @note Calculates kilograms based on load cell specifications, line of best fit, and tare offset

    @ret Current kilogram measurement (float)
*/
float read_kgs(void) {
    float sense_voltage = adc_read_voltage(); // measured voltage
#ifdef DEBUG_OUTPUT
    printf("measured voltage: %f\n", sense_voltage);
#endif
    float kilograms = sense_voltage*(sg.capacity/(sg.VE*sg.RO));
#ifdef DEBUG_OUTPUT
    printf("kilograms: %f\n", kilograms);
#endif	
    
    // if calibrating, we don't have a slope or intercept yet
    if(calibrating)
	return kilograms;

    // manipulate kilograms based on best fit equation
    if(kilograms > 0)
	kilograms = slope * kilograms + intercept;
    else
	kilograms = slope * kilograms - intercept;
    
    // if we're taring, don't consider previous offset
    if(taring)
	return kilograms;
    
    if(sg.offset > 0)
	return kilograms - sg.offset;
    else
	return kilograms + sg.offset;
	
}

/*
    @brief Function for reading pound measurement from strain gauge

    @note Calculates pounds based on kilogram measurement

    @ret Current pound measurement (float)
*/
float read_lbs(void) {
    float kgs = read_kgs();
    return kgs/0.45359237; // kg to lb conversion
}

/*
    @brief Function for reading an average measurement

    @note Uses the external read_sg flag that's set on a timer interrupt to call read_kgs()
	so that there are no timing issues, and no delays are used

    @param[in] times How many times to sample the strain gauge for the average

    @ret Average kg measurement
*/
float read_average(uint8_t times) {
    float sum = 0;
    float weight;
    for(uint8_t i = 0; i < times; i++) {
	while(!read_sg) {} // wait for timer interrupt
	weight = read_kgs();
	read_sg = false; // reset flag
	sum += weight;
#ifdef DEBUG_OUTPUT
	printf("weight: %f, sum: %f\n", weight, sum);
#endif	
    }
#ifdef DEBUG_OUTPUT
	printf("average: %f\n", sum/times);
#endif	
    return sum/times;
}


/*
    @brief Tare the strain gauge

    @note Calculates the average measurement and sets that as the offset to get the reading to 0.
*/
void strain_gauge_tare(void) {
#ifdef DEBUG_OUTPUT
    printf("taring...\n");
#endif
    taring = true;
    float tare_weight = read_average(15); // calculate tare weight
    sg.offset = tare_weight; // set offset
    taring = false;
}

/*
    @brief Calibrate the strain gauge with known weights

    @note Takes an array of known weight values, and calculates a line of best fit equation using
	the known weights as y data and the measured averages per weight as x data. 

    @param[in] weight_cnt Number of known weights

    @param[in] known_weights Float array of known weight values

    @param[in] equation Float array to populate with slope and intercept of line of best fit equation
*/
void strain_gauge_calibrate(uint8_t weight_cnt, float * known_weights, float * equation) {
    calibrating = true;
    uint8_t i;
    float m = 0, b = 0; // slope and intercept
    float sumX = 0, sumX2 = 0, sumY = 0, sumXY = 0;
    float x[weight_cnt+1]; // x data
    float y[weight_cnt+1]; // y data
    y[0] = 0;
    // copy known_weights into y
    uint8_t n = weight_cnt + 1;
    for(i = 0; i <= weight_cnt; i++) {
	y[i+1] = known_weights[i];
    }
    float kilograms = 0;
#ifdef DEBUG_OUTPUT
    printf("Averaging 0 weight, please wait.\n");
#endif
    kilograms = read_average(20);
    x[0] = kilograms;
#ifdef DEBUG_OUTPUT
	printf("%fkg: %f\n", y[0], x[0]);
#endif
    for(i = 1; i <= weight_cnt; i++) {
#ifdef DEBUG_OUTPUT
	printf("You have 15 seconds to put weight %d on (or take it off).\n", i);
#endif
	delay_ms(15000); // wait 15s
	kilograms = read_average(20);
	x[i] = kilograms;
#ifdef DEBUG_OUTPUT
	printf("%fkg: %f\n", y[i], x[i]);
#endif
    }
    
    strain_gauge_calculate_equation(weight_cnt+1, x, y, equation);
}

/*
    @brief Calculate the line of best fit equation given x and y data points

    @param[in] weight_cnt Number of known weights

    @param[in] x X data points, should be measured averages

    @param[in] y Y data points, should be known weights

    @param[in] equation Pointer to a float array to populate with calibration factors
*/
void strain_gauge_calculate_equation(uint8_t weight_cnt, float * x, float * y, float * equation) {
    float m = 0, b = 0; // slope and intercept
    float sumX = 0, sumX2 = 0, sumY = 0, sumXY = 0;
    uint8_t n = weight_cnt + 1;
    // calculate required sums
    for(uint8_t i = 0; i < n; i++) {
	sumX += x[i];
	sumX2 += x[i] * x[i];
	sumY += y[i];
	sumXY += x[i] * y[i];
    }
    // calculate slope and intercept
    m = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    b = (sumY - m * sumX) / n;
#ifdef DEBUG_OUTPUT
    printf("slope: %f, intercept: %f\n", m, b);
#endif
    equation[0] = m;
    equation[1] = b;
}

/*
    @brief Set line of best fit equation

    @note Sets calibration factors

    @param[in] m Slope for line of best fit equation

    @param[in] b Intercept for line of best fit equation
*/
void strain_gauge_set_equation(float m, float b) {
    slope = m;
    intercept = b;
}
