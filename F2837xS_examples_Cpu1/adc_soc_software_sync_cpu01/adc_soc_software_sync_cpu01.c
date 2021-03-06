//###########################################################################
// FILE:   adc_soc_software_sync_cpu01.c
// TITLE:  ADC synchronous software triggering for F2837xS.
//
//! \addtogroup cpu01_example_list
//! <h1> ADC Synchronous SOC Software Force (adc_soc_software_sync)</h1>
//!
//! This example converts some voltages on ADCA and ADCB using input 5 of the
//! input X-BAR as a software force. Input 5 is triggered by toggling GPIO0,
//! but any spare GPIO could be used. This method will ensure that both ADCs
//! start converting at exactly the same time.
//!
//! After the program runs, the memory will contain:
//!
//! - \b AdcaResult0 \b: a digital representation of the voltage on pin A0\n
//! - \b AdcaResult1 \b: a digital representation of the voltage on pin A1\n
//! - \b AdcbResult0 \b: a digital representation of the voltage on pin B0\n
//! - \b AdcbResult1 \b: a digital representation of the voltage on pin B1\n
//!
//
//###########################################################################
// $TI Release: F2837xS Support Library v191 $
// $Release Date: Fri Mar 11 15:58:35 CST 2016 $
// $Copyright: Copyright (C) 2014-2016 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

#include "F28x_Project.h"     // Device Headerfile and Examples Include File

void ConfigureADC(void);
void SetupADCSoftwareSync(void);
void SetupInputXBAR5(void);

//variables to store conversion results
Uint16 AdcaResult0;
Uint16 AdcaResult1;
Uint16 AdcbResult0;
Uint16 AdcbResult1;

void main(void)
{
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xS_SysCtrl.c file.
    InitSysCtrl();

// Step 2. Initialize GPIO:
// This example function is found in the F2837xS_Gpio.c file and
// illustrates how to set the GPIO to it's default state.
    InitGpio();

// Step 3. Clear all interrupts and initialize PIE vector table:
// Disable CPU interrupts
    DINT;

// Initialize the PIE control registers to their default state.
// The default state is all PIE interrupts disabled and flags
// are cleared.
// This function is found in the F2837xS_PieCtrl.c file.
    InitPieCtrl();

// Disable CPU interrupts and clear all CPU interrupt flags:
    IER = 0x0000;
    IFR = 0x0000;

// Initialize the PIE vector table with pointers to the shell Interrupt
// Service Routines (ISR).
// This will populate the entire table, even if the interrupt
// is not used in this example.  This is useful for debug purposes.
// The shell ISR routines are found in F2837xS_DefaultIsr.c.
// This function is found in F2837xS_PieVect.c.
    InitPieVectTable();

// Enable global Interrupts and higher priority real-time debug events:
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

//Configure the ADCs and power them up
    ConfigureADC();

//Setup the ADCs for software conversions
    SetupADCSoftwareSync();

    SetupInputXBAR5();

//take conversions indefinitely in loop
    do
    {
    //convert, wait for completion, and store results

    	//Toggle GPIO0 in software.  This will cause a trigger to
    	//both ADCs via input XBAR, line 5.
    	GpioDataRegs.GPADAT.bit.GPIO0 = 1;
    	GpioDataRegs.GPADAT.bit.GPIO0 = 0;

    	//wait for ADCA to complete, then acknowledge the flag
    	//since both ADCs are running synchronously, it isn't necessary
    	//to wait for completion notification from both ADCs
    	while(AdcaRegs.ADCINTFLG.bit.ADCINT1 == 0);
    	AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

    	//store results
    	AdcaResult0 = AdcaResultRegs.ADCRESULT0;
    	AdcaResult1 = AdcaResultRegs.ADCRESULT1;
    	AdcbResult0 = AdcbResultRegs.ADCRESULT0;
    	AdcbResult1 = AdcbResultRegs.ADCRESULT1;

    	//at this point, conversion results are stored in
    	//AdcaResult0, AdcaResult1, AdcbResult0, and AdcbResult1

    	//software breakpoint, hit run again to get updated conversions
    	asm("   ESTOP0");

    }while(1);
}

//Write ADC configurations and power up the ADC for both ADC A and ADC B
void ConfigureADC(void)
{
	EALLOW;

	//write configurations
	AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
	AdcbRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    AdcSetMode(ADC_ADCB, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);

	//Set pulse positions to late
	AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;
	AdcbRegs.ADCCTL1.bit.INTPULSEPOS = 1;

	//power up the ADCs
	AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
	AdcbRegs.ADCCTL1.bit.ADCPWDNZ = 1;

	//delay for 1ms to allow ADC time to power up
	DELAY_US(1000);

	EDIS;
}

void SetupInputXBAR5(void)
{
	//Setup GPIO 0 to trigger input XBAR line 5.  GPIO0 is used as an example,
	//but any spare GPIO could be used.  The physical GPIO pin should be
	//allowed to float, since the code configures it as an output and changes
	//the value.

	EALLOW;
	InputXbarRegs.INPUT5SELECT = 0; //GPIO0 will trigger the input XBAR line 5

	//GPIO0 as an output
	GPIO_SetupPinOptions(0, GPIO_OUTPUT, GPIO_PUSHPULL);
	GPIO_SetupPinMux(0, GPIO_MUX_CPU1, 0);

	//GPIO0 set as low
	GpioDataRegs.GPADAT.bit.GPIO0 = 0;
}

void SetupADCSoftwareSync(void)
{
	Uint16 acqps;

	//determine minimum acquisition window (in SYSCLKS) based on resolution
	if(ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION)
    {
		acqps = 14; //75ns
	}
	else
    { //resolution is 16-bit
		acqps = 63; //320ns
	}

//Select the channels to convert and end of conversion flag
    //ADCA
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;  //SOC0 will convert pin A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 4; //line 5 of the input X-BAR will trigger the ADC
    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;  //SOC1 will convert pin A1
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 4; //line 5 of the input X-BAR will trigger the ADC
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 1; //end of SOC1 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
    //ADCB
    AdcbRegs.ADCSOC0CTL.bit.CHSEL = 0;  //SOC0 will convert pin B0
    AdcbRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcbRegs.ADCSOC0CTL.bit.TRIGSEL = 4; //line 5 of the input X-BAR will trigger the ADC
    AdcbRegs.ADCSOC1CTL.bit.CHSEL = 1;  //SOC1 will convert pin B1
    AdcbRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcbRegs.ADCSOC1CTL.bit.TRIGSEL = 4; //line 5 of the input X-BAR will trigger the ADC
}
