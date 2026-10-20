/**
 * Generated Driver File
 * 
 * @file pins.c
 * 
 * @ingroup  pinsdriver
 * 
 * @brief This is generated driver implementation for pins. 
 *        This file provides implementations for pin APIs for all pins selected in the GUI.
 *
 * @version Driver Version 1.1.0
*/

/*
© [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#include "../pins.h"

static void (*BTN_InterruptHandler)(void);
static void (*CDATA_InterruptHandler)(void);
static void (*CCLK_InterruptHandler)(void);
static void (*CREQ_InterruptHandler)(void);
static void (*LED1_InterruptHandler)(void);
static void (*LED2_InterruptHandler)(void);

void PIN_MANAGER_Initialize()
{

  /* OUT Registers Initialization */
    PORTA.OUT = 0x0;
    PORTC.OUT = 0x0;
    PORTD.OUT = 0x0;
    PORTF.OUT = 0x0;

  /* DIR Registers Initialization */
    PORTA.DIR = 0x1;
    PORTC.DIR = 0x8;
    PORTD.DIR = 0x0;
    PORTF.DIR = 0x0;

  /* PINxCTRL registers Initialization */
    PORTA.PIN0CTRL = 0x0;
    PORTA.PIN1CTRL = 0x0;
    PORTA.PIN2CTRL = 0x0;
    PORTA.PIN3CTRL = 0x0;
    PORTA.PIN4CTRL = 0x0;
    PORTA.PIN5CTRL = 0x0;
    PORTA.PIN6CTRL = 0x0;
    PORTA.PIN7CTRL = 0x0;
    PORTC.PIN0CTRL = 0x0;
    PORTC.PIN1CTRL = 0x0;
    PORTC.PIN2CTRL = 0x0;
    PORTC.PIN3CTRL = 0x0;
    PORTC.PIN4CTRL = 0x0;
    PORTC.PIN5CTRL = 0x0;
    PORTC.PIN6CTRL = 0x0;
    PORTC.PIN7CTRL = 0x0;
    PORTD.PIN0CTRL = 0x0;
    PORTD.PIN1CTRL = 0x0;
    PORTD.PIN2CTRL = 0x0;
    PORTD.PIN3CTRL = 0x0;
    PORTD.PIN4CTRL = 0x0;
    PORTD.PIN5CTRL = 0x0;
    PORTD.PIN6CTRL = 0x0;
    PORTD.PIN7CTRL = 0x0;
    PORTF.PIN0CTRL = 0x0;
    PORTF.PIN1CTRL = 0x0;
    PORTF.PIN2CTRL = 0x0;
    PORTF.PIN3CTRL = 0x0;
    PORTF.PIN4CTRL = 0x0;
    PORTF.PIN5CTRL = 0x0;
    PORTF.PIN6CTRL = 0x0;
    PORTF.PIN7CTRL = 0x0;

  /* PORTMUX Initialization */
    PORTMUX.CCLROUTEA = 0x0;
    PORTMUX.EVSYSROUTEA = 0x0;
    PORTMUX.SPIROUTEA = 0x0;
    PORTMUX.TCAROUTEA = 0x0;
    PORTMUX.TWIROUTEA = 0x0;
    PORTMUX.USARTROUTEA = 0x0;

  // register default ISC callback functions at runtime; use these methods to register a custom function
    BTN_SetInterruptHandler(BTN_DefaultInterruptHandler);
    CDATA_SetInterruptHandler(CDATA_DefaultInterruptHandler);
    CCLK_SetInterruptHandler(CCLK_DefaultInterruptHandler);
    CREQ_SetInterruptHandler(CREQ_DefaultInterruptHandler);
    LED1_SetInterruptHandler(LED1_DefaultInterruptHandler);
    LED2_SetInterruptHandler(LED2_DefaultInterruptHandler);
}

/**
  Allows selecting an interrupt handler for BTN at application runtime
*/
void BTN_SetInterruptHandler(void (* interruptHandler)(void)) 
{
    BTN_InterruptHandler = interruptHandler;
}

void BTN_DefaultInterruptHandler(void)
{
    // add your BTN interrupt custom code
    // or set custom function using BTN_SetInterruptHandler()
}
/**
  Allows selecting an interrupt handler for CDATA at application runtime
*/
void CDATA_SetInterruptHandler(void (* interruptHandler)(void)) 
{
    CDATA_InterruptHandler = interruptHandler;
}

void CDATA_DefaultInterruptHandler(void)
{
    // add your CDATA interrupt custom code
    // or set custom function using CDATA_SetInterruptHandler()
}
/**
  Allows selecting an interrupt handler for CCLK at application runtime
*/
void CCLK_SetInterruptHandler(void (* interruptHandler)(void)) 
{
    CCLK_InterruptHandler = interruptHandler;
}

void CCLK_DefaultInterruptHandler(void)
{
    // add your CCLK interrupt custom code
    // or set custom function using CCLK_SetInterruptHandler()
}
/**
  Allows selecting an interrupt handler for CREQ at application runtime
*/
void CREQ_SetInterruptHandler(void (* interruptHandler)(void)) 
{
    CREQ_InterruptHandler = interruptHandler;
}

void CREQ_DefaultInterruptHandler(void)
{
    // add your CREQ interrupt custom code
    // or set custom function using CREQ_SetInterruptHandler()
}
/**
  Allows selecting an interrupt handler for LED1 at application runtime
*/
void LED1_SetInterruptHandler(void (* interruptHandler)(void)) 
{
    LED1_InterruptHandler = interruptHandler;
}

void LED1_DefaultInterruptHandler(void)
{
    // add your LED1 interrupt custom code
    // or set custom function using LED1_SetInterruptHandler()
}
/**
  Allows selecting an interrupt handler for LED2 at application runtime
*/
void LED2_SetInterruptHandler(void (* interruptHandler)(void)) 
{
    LED2_InterruptHandler = interruptHandler;
}

void LED2_DefaultInterruptHandler(void)
{
    // add your LED2 interrupt custom code
    // or set custom function using LED2_SetInterruptHandler()
}
ISR(PORTA_PORT_vect)
{ 
    // Call the interrupt handler for the callback registered at runtime
    if(VPORTA.INTFLAGS & PORT_INT1_bm)
    {
       BTN_InterruptHandler(); 
    }
    if(VPORTA.INTFLAGS & PORT_INT0_bm)
    {
       LED1_InterruptHandler(); 
    }
    /* Clear interrupt flags */
    VPORTA.INTFLAGS = 0xff;
}

ISR(PORTC_PORT_vect)
{ 
    // Call the interrupt handler for the callback registered at runtime
    if(VPORTC.INTFLAGS & PORT_INT3_bm)
    {
       LED2_InterruptHandler(); 
    }
    /* Clear interrupt flags */
    VPORTC.INTFLAGS = 0xff;
}

ISR(PORTD_PORT_vect)
{ 
    // Call the interrupt handler for the callback registered at runtime
    if(VPORTD.INTFLAGS & PORT_INT4_bm)
    {
       CDATA_InterruptHandler(); 
    }
    if(VPORTD.INTFLAGS & PORT_INT5_bm)
    {
       CCLK_InterruptHandler(); 
    }
    if(VPORTD.INTFLAGS & PORT_INT6_bm)
    {
       CREQ_InterruptHandler(); 
    }
    /* Clear interrupt flags */
    VPORTD.INTFLAGS = 0xff;
}

ISR(PORTF_PORT_vect)
{ 
    /* Clear interrupt flags */
    VPORTF.INTFLAGS = 0xff;
}

/**
 End of File
*/