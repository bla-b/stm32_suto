#include "ads124s08.h"


/*
Configure microcontroller for SPI mode 1 (CPOL = 0, CPHA = 1)
Configure microcontroller GPIO for /DRDY as a falling edge triggered interrupt input
Set CS low;
Send 06;// RESET command to make sure the device is properly reset after power-up
Set CS high;
Set CS low;// Configure the device
Send 42// WREG starting at 02h address
05// Write to 6 registers
12// Select AINP = AIN1 and AINN = AIN2
0A// PGA enabled, Gain = 4
14// Continuous conversion mode, low-latency filter, 20-SPS data rate
12// Positive reference buffer enabled, negative reference buffer disabled
 // REFP0 and REFN0 reference selected, internal reference always on
07// IDAC magnitude set to 1 mA
F0;// IDAC1 set to AIN0, IDAC2 disabled
Set CS high;
Set CS low; // For verification, read back configuration registers
Send 22// RREG starting at 02h address
05// Read from 6 registers
00 00 00 00 00 00;// Send 6 NOPs for the read
Set CS high;
Set CS low;
Send 08;// Send START command to start converting in continuous conversion mode;
Set CS high;
Loop
{
Wait for DRDY to transition low;
Set CS low;
Send 12// Send RDATA command
00 00 00;// Send 3 NOPs (24 SCLKs) to clock out data
Set CS high;
}
Set CS low;
Send 0A;//STOP command stops conversions and puts the device in standby mode;
Set CS to high;
*/
void ads_init();
