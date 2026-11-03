 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
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
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/timer_interface.h"
#include "mcc_generated_files/timer/tca0.h"
#include <usb_core.h>
#include <usb_hid_keycodes.h>
#include <usb_hid_keyboard.h>

#include <stdint.h>
#include "scheduler.h"
#include "button.h"

volatile uint16_t sysTick;

/* ---------------------------------------------------------------------- */
/* Task identifiers: index into mainTasks[] below                         */
/* ---------------------------------------------------------------------- */
enum {
    mainTaskIdBlink = 0,
    mainTaskIdButton,
    mainTaskIdkeyboard,
    mainTaskIdCount
};

/* ---------------------------------------------------------------------- */
/* Button identifiers: index into mainButtons[] below                     */
/* ---------------------------------------------------------------------- */
enum {
    mainButtonIdUser = 0,
    mainButtonIdCount
};
 

typedef enum {
    TX_IDLE,
    TX_START,
    TX_KEY_DOWN,
    TX_KEY_UP,
} txState_t;

static uint8_t keyboardBuf[16] = { HID_H, HID_E, HID_L, HID_L, HID_O };
static uint8_t keyboardBufLen = 5;

static txState_t txState = TX_IDLE;
static uint8_t   txIndex = 0;


static void keyboardTask(void)
{
    switch (txState) {

        case TX_IDLE:
            break;
        case TX_START:
            txState = TX_KEY_DOWN;
            txIndex = 0;
            break;    
        case TX_KEY_DOWN:
            if (USB_HIDKeyPressDown(keyboardBuf[txIndex]) == SUCCESS) {
                txState = TX_KEY_UP;
            }
            break;
        case TX_KEY_UP:
            if (USB_HIDKeyPressUp(keyboardBuf[txIndex]) == SUCCESS) {            
                txIndex++;
                if (txIndex == keyboardBufLen) {
                    txState = TX_IDLE;
                } else {
                    txState = TX_KEY_DOWN;
                }
            }
            break;
    }
}



/* ---------------------------------------------------------------------- */
/* Button functions                                                       */
/* ---------------------------------------------------------------------- */
static uint8_t readButtonUser(void)
{
    /* Return non-zero while pressed. */
    return BTN_GetValue() == 0;
}

static void onButtonPressed(uint8_t id)
{
    (void)id;
    
    if (txState == TX_IDLE) {
        txState = TX_START;
    }       
}
 
static void onButtonHeld(uint8_t id, uint32_t heldMs)
{
    (void)id;
}
 
static void onButtonReleased(uint8_t id, uint32_t heldMs)
{
    (void)id;
}

/* ---------------------------------------------------------------------- */
/* Button table                                                           */
/* ---------------------------------------------------------------------- */
static button mainButtons[mainButtonIdCount] = {
    { BUTTONINIT(readButtonUser, onButtonPressed, onButtonHeld, onButtonReleased) },
};

/* ---------------------------------------------------------------------- */
/* Task implementations                                                   */
/* ---------------------------------------------------------------------- */
static void blinkTask(void)
{
    /* toggle LED here */
    
    LED1_Toggle();
}

/* ---------------------------------------------------------------------- */
/* Task table                                                             */
/* nextRun = delay in ticks before the first run (for enabled tasks)      */
/* ---------------------------------------------------------------------- */
static schedulerTask mainTasks[mainTaskIdCount] = {
    /* func        nextRun period remaining state                    */
    { blinkTask,    0,  SCHEDULERMS(500),   0,    schedulerTaskEnabled  },  /* every 500 ms */
    { buttonTask,   0,  BUTTONPOLLPERIOD,   0,    schedulerTaskEnabled  },  /* every 10 ms  */
    { keyboardTask, 0,  SCHEDULERMS(10),    0,    schedulerTaskEnabled  },  /* every 10 ms */    
};


/* ---------------------------------------------------------------------- */
/* Timer callback, called every 10ms                                      */
/* ---------------------------------------------------------------------- */
void timerCallbackHandler(void) {
    sysTick++;
}


// USB status variable
RETURN_CODE_t status;

/* ---------------------------------------------------------------------- */
/* Main application                                                       */
/* ---------------------------------------------------------------------- */
int main(void)
{
    SYSTEM_Initialize();

    buttonInit(mainButtons, mainButtonIdCount);
    schedulerInit(mainTasks, mainTaskIdCount);
        
    Timer0.TimeoutCallbackRegister(timerCallbackHandler);
    Timer0.Start();    
    
    while(1)
    {
        status = USBDevice_Handle();
        if (status != SUCCESS)
        {
            (void)USB_Stop();        // detach + disable peripheral
            USBDevice_Initialize();  // re-arms the stack and sets usbStatus again
        }
        
        schedulerRun();
    }    
}