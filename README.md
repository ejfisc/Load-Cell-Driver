# Load Cell (Strain Gauge) Driver

## Getting Started
This driver uses the SparkFun HX711F driver that's in [a separate repo on my profile](https://github.com/ejfisc/HX711F-Load-Cell-Amp-Driver). 

The `delay_ms()` macro is currently using the Nordic nRF5 SDK specific millisecond delay function, you'll have to modify this if you're using a different micro. 

Determine the capacity (in kg) and rated output (in mV/V) of your load cell as well as the excitation voltage (Vin) that you're supplying the load cell with. Then call `strain_gauge_init()`. 

## Calibration
Assuming you have a set of calibration weights or some number of known weights, calibration is fairly simple. Essentially what you'll do is place one weight on the load cell at a time and and allow the load cell to calculate an average reading for each weight. The averages of the measured weights and known weight values are used to calculate a line of best fit equation that is used in `read_kgs()` that will ensure your load cell is giving you accurate readings. When the calibration sequence is done correctly, I was able to get the measurements within 1% error.  

Here is the psuedocode example for calibrating:
```
float equation[2];
float known_weights[10] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
strain_gauge_calibrate(10, known_weights, equation);
strain_gauge_set_equation(equation[0], equation[1]);
```
Do not include 0 weight in the known weights array, the calibration function handles 0 weight. 

You'll need to turn debug output on for the calibration sequence, as it will tell you when to put the next weight on. 

It is recommended you use flash storage to save the calibration factors so that you don't have to recalibration the load cell every time you reprogram the micro or power on your system. 

## Debug Output
A precompiler directive is used to turn debug output on and off. Currently all of the outputs are using `printf()`, change these to whatever your micro / dev environment uses.