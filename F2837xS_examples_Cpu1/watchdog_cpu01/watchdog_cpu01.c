//###########################################################################
// FILE:   watchdog_cpu01.c
// TITLE:  watchdog Example for F2837xS.
//
//! \addtogroup cpu01_example_list
//! <h1> Watchdog </h1>
//!
//! This example shows how to service the watchdog or generate a wakeup
//! interrupt using the watchdog. By default the example will generate a
//! Wake interrupt.  To service the watchdog and not generate the interrupt
//! uncomment the ServiceDog() line the the main for loop.
//
//###########################################################################
// $TI Release: F2837xS Support Library v191 $
// $Release Date: Fri Mar 11 15:58:35 CST 2016 $
// $Copyright: Copyright (C) 2014-2016 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

#include "F28x_Project.h"     // Device Headerfile and Examples Include File

// Prototype statements for functions found within this file.
__interrupt void wakeint_isr(void);

// Global variables for this example
Uint32 WakeCount;
Uint32 LoopCount;

void main(void)
{

//   unsigned long delay;
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xS_SysCtrl.c file.
    InitSysCtrl();

// Step 2. Initialize GPIO:
// This example function is found in the F2837xS_Gpio.c file and
// illustrates how to set the GPIO to it's default state.
// InitGpio();

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

    // Interrupts that are used in this example are re-mapped to
    // ISR functions found within this file.
	EALLOW;  // This is needed to write to EALLOW protected registers
	PieVectTable.WAKE_INT = &wakeint_isr;
	EDIS;    // This is needed to disable write to EALLOW protected registers

// Step 4. User specific code, enable interrupts:
// Clear the counters
	WakeCount = 0; // Count interrupts
	LoopCount = 0; // Count times through idle loop
// Connect the watchdog to the WAKEINT interrupt of the PIE
// Write to the whole SCSR register to avoid clearing WDOVERRIDE bit
	EALLOW;
	WdRegs.SCSR.all = BIT1;
	EDIS;

// Enable WAKEINT in the PIE: Group 1 interrupt 8
// Enable INT1 which is connected to WAKEINT:
	PieCtrlRegs.PIECTRL.bit.ENPIE = 1;   // Enable the PIE block
	PieCtrlRegs.PIEIER1.bit.INTx8 = 1;   // Enable PIE Group 1 INT8
	IER |= M_INT1;                       // Enable CPU INT1
	EINT;                                // Enable Global Interrupts

// Reset the watchdog counter
	ServiceDog();

// Enable the watchdog
	EALLOW;
	WdRegs.WDCR.all = 0x0028;
	EDIS;


// Step 6. IDLE loop. Just sit and loop forever (optional):
    for(;;)
    {
        LoopCount++;

        // Uncomment ServiceDog to just loop here
        // Comment ServiceDog to take the WAKEINT instead
        // ServiceDog();
    }
}


// Step 7. Insert all local Interrupt Service Routines (ISRs) and functions here:
// If local ISRs are used, reassign vector addresses in vector table as
// shown in Step 5

__interrupt void wakeint_isr(void)
{
	WakeCount++;

	// Acknowledge this interrupt to get more from group 1
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//===========================================================================
// No more.
//===========================================================================
