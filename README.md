# Arduino-Speedometer
// Simple Speedometer
// This is a simple, inexpensive speedometer for model railoads
// It uses an Arduino UNO or similar
// a 16x2 I2C LCD display 
// and two IR proximity sensors
// it detects the speed of a train moving either direction on a single track mainline
// constants are provided for calciulating the speed in several popular scales, 
// and in MPH or km/h
// Displayed text is factored out so it can be localized 
// or easily changed for individual preferences

// Robert Myers 2023 
// This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.

// the proper values of SPEED_DIVISOR, LENGTH,
// SPEEDLABEL and CAR_GAP 
// must be set for the desured scale, sensor distance,
// speed label (MPH, or km/h)
// and allowance for gaps between cars
