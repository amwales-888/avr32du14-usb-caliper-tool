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


// Standard library includes
#include <stdbool.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>

// Project specific headers
#include <stdint.h>
#include "scheduler.h"
#include "button.h"
#include "spc.h"

volatile uint16_t sysTick;

/* ---------------------------------------------------------------------- */
/* Task identifiers: index into mainTasks[] below                         */
/* ---------------------------------------------------------------------- */
enum {
    mainTaskIdSystem = 0,
    mainTaskIdUSB,
    mainTaskIdBlink,
    mainTaskIdButton,
    mainTaskIdkeyboard,
    mainTaskIdSPC,
    mainTaskIdCount
};

/* ---------------------------------------------------------------------- */
/* Button identifiers: index into mainButtons[] below                     */
/* ---------------------------------------------------------------------- */
enum {
    mainButtonIdUser = 0,
    mainButtonIdCount
};
 
/* ---------------------------------------------------------------------- */
/* SPC states                                                             */
/* ---------------------------------------------------------------------- */
typedef enum {
    SPCTASK_IDLE = 0,
    SPCTASK_START,
    SPCTASK_PENDING,
    SPCTASK_ERROR,
    SPCTASK_OK,
} spcState_t;

static spcState_t spcState = SPCTASK_IDLE;

/* ---------------------------------------------------------------------- */
/* HID Keyboard text output states                                        */
/* ---------------------------------------------------------------------- */
typedef enum {
    HIDTASK_IDLE = 0,
    HIDTASK_START,
    HIDTSAK_KEY_DOWN,
    HIDTASK_KEY_UP,
} txState_t;

static txState_t txState = HIDTASK_IDLE;
static uint8_t   txIndex = 0;

static uint8_t keyboardBuf[16] = { HID_H, HID_E, HID_L, HID_L, HID_O };
static uint8_t keyboardBufLen = 5;

/* ---------------------------------------------------------------------- */
/* HID Keyboard Task                                                      */
/* ---------------------------------------------------------------------- */
static void keyboardTask(void)
{
    switch (txState) {

        case HIDTASK_IDLE:
            break;
        case HIDTASK_START:
            txState = HIDTSAK_KEY_DOWN;
            txIndex = 0;
            break;    
        case HIDTSAK_KEY_DOWN:
            if (USB_HIDKeyPressDown(keyboardBuf[txIndex]) == SUCCESS) {
                txState = HIDTASK_KEY_UP;
            } else {
                /* check here for timeout */
            }
            break;
        case HIDTASK_KEY_UP:
            if (USB_HIDKeyPressUp(keyboardBuf[txIndex]) == SUCCESS) {            
                txIndex++;
                if (txIndex == keyboardBufLen) {
                    spcState = SPCTASK_IDLE;
                    txState = HIDTASK_IDLE;
                } else {
                    txState = HIDTSAK_KEY_DOWN;
                }
            } else {
                /* check here for timeout */
            }
            break;
    }
}

/* ---------------------------------------------------------------------- */
/* Reading SPC Task                                                       */
/* ---------------------------------------------------------------------- */

static uint32_t nowMS;

static void systemTask(void)
{
    /* Update system time MS */
    nowMS = sysTick * 10;
}

/* ---------------------------------------------------------------------- */
/* Reading SPC Task                                                       */
/* ---------------------------------------------------------------------- */

static spc_reading_t spcData;

static void readSpcTask(void)
{
    switch (spcState) {

        case SPCTASK_IDLE:
            break;
        case SPCTASK_START:
            spcState = SPCTASK_PENDING;            
            break;    
        case SPCTASK_PENDING: {

            spc_status_t spc_status;
            
            spc_status = spcRead(&spcData, nowMS);
            switch (spc_status) {
                
                case SPC_OK:
                    spcState = SPCTASK_OK;
                    break;
                case SPC_PENDING:
                    break;
                case SPC_ERR_TIMEOUT:
                    spcState = SPCTASK_ERROR;
                    break;
                case SPC_ERR_FORMAT:
                    spcState = SPCTASK_ERROR;
                    break;                                
            }
            
            break;
        }
        case SPCTASK_ERROR:
            spcState = SPCTASK_IDLE;
            break;
        case SPCTASK_OK:            

            /* format the data and output it */            
            txState = HIDTASK_START;
            break;
    }
}

/* ---------------------------------------------------------------------- */
/* Handle USB Task                                                       */
/* ---------------------------------------------------------------------- */

// USB status variable
RETURN_CODE_t status;

static void handleUSBTask(void)
{
    status = USBDevice_Handle();
    if (status != SUCCESS) {

        (void)USB_Stop();        // detach + disable peripheral
        USBDevice_Initialize();  // re-arms the stack and sets usbStatus again
    }
}

/* ---------------------------------------------------------------------- */
/* Blink Task                                                             */
/* ---------------------------------------------------------------------- */
static void blinkTask(void)
{
    /* toggle LED here */    
    LED1_Toggle();
}


/* ---------------------------------------------------------------------- */
/* Button callback functions                                              */
/* ---------------------------------------------------------------------- */
static uint8_t readButtonUser(void)
{
    /* Return non-zero while pressed. */
    return BTN_GetValue() == 0;
}

static void onButtonPressed(uint8_t id)
{
    (void)id;
    
//    if (txState == TX_IDLE) {
//        txState = TX_START;
//    }       
        
    if (spcState == SPCTASK_IDLE) {
        spcState = SPCTASK_START;
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
/* Task table                                                             */
/* nextRun = delay in ticks before the first run (for enabled tasks)      */
/* ---------------------------------------------------------------------- */
static schedulerTask mainTasks[mainTaskIdCount] = {
    /* func        nextRun period remaining state                    */
    { systemTask,    0,  SCHEDULERMS(10),    0,    schedulerTaskEnabled },  /* every 10 ms */
    { handleUSBTask, 0,  SCHEDULERALWAYS,    0,    schedulerTaskEnabled },  /* every schedulerRun */
    { blinkTask,     0,  SCHEDULERMS(500),   0,    schedulerTaskEnabled },  /* every 500 ms */
    { buttonTask,    0,  BUTTONPOLLPERIOD,   0,    schedulerTaskEnabled },  /* every 10 ms  */
    { keyboardTask,  0,  SCHEDULERMS(10),    0,    schedulerTaskEnabled },  /* every 10 ms */    
    { readSpcTask,   0,  SCHEDULERALWAYS,    0,    schedulerTaskEnabled },  /* every schedulerRun */
};


/* ---------------------------------------------------------------------- */
/* Timer callback, called every 10ms                                      */
/* ---------------------------------------------------------------------- */
void timerCallbackHandler(void) {
    sysTick++;
}

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
    
    while(1) {
         schedulerRun();
    }    
}