# Analog-FRAM-RTC

Code for firmware and simple wireframes for the Androd App


Some background on the project:

The sensors we are connecting to have a 4-20mA analog current output. This 4-20 range corresponds to an instantaneous Power (HP or kW) level of the motor connectes to the sensor. So a 4mA output is 0HP, and a 20mA output level is the full scale of the programmed HP, usually the full HP capacity of the motor or slightly more.
When looking at the terminal strip on the bottom of the board, the 4-20mA connections are on the right. It's a current loop so polarity shouldn't matter.
The terminal strip onthe left is a 24V power connection (-)on the left, (+) on the right.

The current code stores the 4-20 data point, averaged over the sample period, along with a time stamp from the Real Time Clock. The formats are:
4-20mA = 4 digit integer from 4000 to 20000. This is calculated by doing an A2D conversion of the input on A0 which ranges from .33V to 3.33V
Year = 4 digit integer of the year (e.g., 2021)
Date = 4 digit integer of the date (e.g., 0824)
Hour_Min = 4 digit integer of the time (e.g., 1325)

Note: we don't handle daylight savings time or time zones currently, Mike needs to research this prior to customer ship

