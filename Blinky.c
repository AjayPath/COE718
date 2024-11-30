/*----------------------------------------------------------------------------
 * Name:    Ajanthan Pathmanathan
 * Purpose: Media Center Final Project
 * Note(s): This code will be taken down from github once the report is marked
 *----------------------------------------------------------------------------*/

// Libraries

#include <stdio.h>   // Standard input/output
#include <stdint.h>   // Standard integer types
#include "string.h"   // String manipulation functions
#include "GLCD.h"     // Graphics LCD functions
#include "LED.h"      // LED control functions
#include "KBD.h"      // Keypad control functions

#include "redbull.c"  // Red Bull audio file
#include "merc.c"     // Merc audio file
#include "haas.c"     // Haas audio file

#include "audioMenu.c" // Audio menu functions

// Audio Libraries

#include "LPC17xx.h"   // LPC17xx specific functions
#include "type.h"      // Type definitions

#include "usb.h"       // USB general functions
#include "usbcfg.h"    // USB configuration functions
#include "usbhw.h"     // USB hardware control functions
#include "usbcore.h"   // USB core functions
#include "usbaudio.h"  // USB audio functions


extern  void SystemClockUpdate(void);    // Update system clock
extern uint32_t SystemFrequency;         // System frequency variable
uint8_t  Mute;                           // Mute status flag
uint32_t Volume;                         // Volume level

#if USB_DMA
uint32_t *InfoBuf = (uint32_t *)(DMA_BUF_ADR);  // DMA buffer for info
short *DataBuf = (short *)(DMA_BUF_ADR + 4*P_C); // DMA buffer for audio data
#else
uint32_t InfoBuf[P_C];                   // Regular buffer for info
short DataBuf[B_S];                      // Regular buffer for audio data
#endif

uint16_t  DataOut;                       // Data output register
uint16_t  DataIn;                        // Data input register

uint8_t   DataRun;                       // Flag to indicate if data is running
uint16_t  PotVal;                        // Potentiometer value
uint32_t  VUM;                           // Volume Unit Meter (VUM)
uint32_t  Tick;                          // System tick counter

// Define joystick button bit values (based on hardware design)
#define KBD_SELECT 0x01   // Select button on joystick
#define KBD_UP     0x08   // Up button
#define KBD_RIGHT  0x10   // Right button
#define KBD_DOWN   0x20   // Down button
#define KBD_LEFT   0x40   // Left button
#define KBD_MASK   0x79   // Mask for the relevant joystick bits (excluding Select)

#define __FI        1                      // Font index for the LCD (16x24 font size)
#define __USE_LCD   1                      // Set to 1 to enable LCD output (disabled by default)

// External variables/functions to interact with the joystick
extern uint32_t KBD_val;                // Joystick value (external)
extern void KBD_Init(void);             // Joystick initialization function
extern uint32_t KBD_get(void);          // Function to get joystick status
extern uint32_t get_button(void);       // Function to get joystick button press
volatile uint32_t kbd_val;              // Global variable to store joystick status (debugging)

int currentImage;                       // Index for the current image
char reactionTimeStr[20];               // String for displaying reaction time

// DELAY FUNCTION USING FOR LOOP
void delay (uint32_t delay_time) {

	volatile uint32_t i;
	
	for (i = 0; i < delay_time; i++) {
	
	}

}

// SLIDE SHOW FUNCTION
void slideshow() {
	
	GLCD_Clear(White);  // Clear the display to white
	
	while (1) {  // Infinite loop for slideshow
		uint32_t joystickInput = get_button();  // Get joystick input
		
		if (joystickInput == KBD_DOWN) {  // If down button is pressed
			currentImage++;  // Move to next image
			GLCD_Clear(White);  // Clear display
			
			if (currentImage > 2) {  // If last image is reached, wrap around
				currentImage = 0;
			}
			
			delay(10000000);  // Delay for button debounce
		}
		
		if (joystickInput == KBD_UP) {  // If up button is pressed
			currentImage--;  // Move to previous image
			GLCD_Clear(White);  // Clear display
			
			if (currentImage < 0) {  // If first image is reached, wrap around
				currentImage = 2;
			}
			
			delay(10000000);  // Delay for button debounce
		}
	
		// Display the current image based on the index
		switch (currentImage) {
		
			case 0:
				GLCD_DisplayString(0, 0, __FI, "     Slideshow!     ");  // Display title
				GLCD_DisplayString(1, 0, __FI, "      IMAGE  1      ");  // Display image number
				GLCD_Bitmap(160-50, 120-48, REDBULL_WIDTH, REDBULL_HEIGHT, REDBULL_PIXEL_DATA);  // Display image 1
				GLCD_DisplayString(9, 0, __FI, " USE ^ and v to CTRL ");  // Display instructions
				break;
			case 1:
				GLCD_DisplayString(0, 0, __FI, "     Slideshow!     ");  // Display title
				GLCD_DisplayString(1, 0, __FI, "      IMAGE  2      ");  // Display image number
				GLCD_Bitmap(160-50, 120-50, MERC_WIDTH, MERC_HEIGHT, MERC_PIXEL_DATA);  // Display image 2
				GLCD_DisplayString(9, 0, __FI, " USE ^ and v to CTRL ");  // Display instructions
				break;
			case 2:
				GLCD_DisplayString(0, 0, __FI, "     Slideshow!     ");  // Display title
				GLCD_DisplayString(1, 0, __FI, "      IMAGE  3      ");  // Display image number
				GLCD_Bitmap(160-50, 120-50, HAAS_WIDTH, HAAS_HEIGHT, HAAS_PIXEL_DATA);  // Display image 3
				GLCD_DisplayString(9, 0, __FI, " USE ^ and v to CTRL ");  // Display instructions
				break;
		}
	
		if (joystickInput == KBD_LEFT) {  // If left button is pressed
			GLCD_Clear(White);  // Clear display
			return;  // Exit the slideshow
		}
	}
	
}

// GET POT VALUE
void get_potval (void) {
  uint32_t val;

  LPC_ADC->CR |= 0x01000000;              /* Start A/D Conversion */
  do {
    val = LPC_ADC->GDR;                   /* Read A/D Data Register */
  } while ((val & 0x80000000) == 0);      /* Wait for end of A/D Conversion */
  LPC_ADC->CR &= ~0x01000000;             /* Stop A/D Conversion */
  PotVal = ((val >> 8) & 0xF8) +          /* Extract Potenciometer Value */
           ((val >> 7) & 0x08);
}

// REACTION TIME GAME FUNCTION
void reactionTimeGame() {
	
	uint32_t joystickInput;
	uint32_t startTime = 0;
    uint32_t reactionTime = 0;
    uint32_t currentTime = 0;
    int time_counter = 0;  // Used for incrementing the time
	
	GLCD_Clear(White);  // Clear the display
    GLCD_SetTextColor(Black);  // Set text color to black
	
	// Main loop for the reaction time game
	while (1) {
		
		GLCD_DisplayString(0, 0, __FI, "   Welcome to the   ");
    	GLCD_DisplayString(1, 0, __FI, " Reaction Time Game ");

    	GLCD_DisplayString(8, 0, __FI, "    Start Game ->   ");
    	GLCD_DisplayString(9, 0, __FI, "   <- Return Home   ");	
		
		joystickInput = get_button();  // Get joystick input
			
		if (joystickInput == KBD_RIGHT) {  // If right button is pressed to start game
			
			GLCD_Clear(White);  // Clear screen for instructions
			GLCD_DisplayString(5, 0, __FI, "PRESS SELECT WHEN IT");
			GLCD_DisplayString(6, 0, __FI, "     GOES BLACK     ");

			delay(80000000);  // Delay for user to read instructions
			GLCD_Clear(Black);  // Change screen color to black to start game	

			GLCD_SetTextColor(Black);  // Set text color to black
	
			// Start measuring reaction time (time between GLCD background change and button press)
			startTime = 0;  // Reset the start time

			// Change background color to signal user
			GLCD_Clear(White);  // Flash the screen with white color

			// Main loop for waiting for button press
			while (1) {
				// Increment the counter (representing time passed)
				delay(1000);  // Adjust this to control the speed of time incrementation
				time_counter++;    // Each loop is one "tick" of time passed
				currentTime = time_counter;

				// Check if the joystick button is pressed
				joystickInput = get_button();
				if (joystickInput == KBD_SELECT) {  // If select button is pressed
					reactionTime = currentTime;  // Store reaction time as the time passed
					break;  // Exit the loop once the button is pressed
				}
			}

			// Display the reaction time as a string
			sprintf(reactionTimeStr, "Reaction Time: %d", reactionTime);  

			// Loop to show results and allow user to restart the game
			while (1) {
				joystickInput = get_button();
				
				GLCD_DisplayString(5, 0, __FI, reactionTimeStr);  // Display reaction time
				
				// Display win/lose message based on reaction time
				if (reactionTime < 500) {
					GLCD_SetTextColor(DarkGreen);  // Set text color to green for win
					GLCD_DisplayString(6, 0, __FI, "      You Win!      ");
					GLCD_DisplayString(9, 0, __FI, "   <- Restart Game   ");	
				}
				
				if (reactionTime > 500) {
					GLCD_SetTextColor(Red);  // Set text color to red for lose
					GLCD_DisplayString(6, 0, __FI, "      You Lose      ");
					GLCD_DisplayString(9, 0, __FI, "   <- Restart Game  ");	
				}
				
				// If left button is pressed, restart the game
				if (joystickInput == KBD_LEFT) {
					reactionTimeGame();  // Restart the game
					return;
				}

            }
		}
		
		// If left button is pressed, return to home screen
		if (joystickInput == KBD_LEFT) {
			GLCD_Clear(White);  // Clear screen
			return;  // Exit the game
		}
	}
}

 void TIMER0_IRQHandler(void) 
{
   long val;
   uint32_t cnt;

   if (DataRun) {  // If data is running
     val = DataBuf[DataOut];  // Get current data value
     cnt = (DataIn - DataOut) & (B_S - 1);  // Calculate data count

     // If buffer is about to wrap around, increment DataOut
     if (cnt == (B_S - P_C*P_S)) {  
       DataOut++;  // Increment DataOut pointer
     }

     // If there is enough data, increment DataOut
     if (cnt > (P_C*P_S)) {  
       DataOut++;  // Increment DataOut pointer
     }
     DataOut &= B_S - 1;  // Ensure DataOut wraps within buffer size

     // Update VUM based on data value
     if (val < 0) VUM -= val;  // If value is negative, subtract from VUM
     else         VUM += val;  // If value is positive, add to VUM

     val *= Volume;  // Apply volume scaling
     val >>= 16;  // Right shift for DAC conversion
     val += 0x8000;  // Adjust to center value for DAC
     val &= 0xFFFF;  // Mask to ensure 16-bit output
   } else {
     val = 0x8000;  // If data isn't running, output neutral value
   }

   if (Mute) {  // If mute is enabled
     val = 0x8000;  // Output neutral value
   }

   LPC_DAC->CR = val & 0xFFC0;  // Send value to DAC

   if ((Tick++ & 0x03FF) == 0) {  // Every 1024 ticks
     get_potval();  // Get the potentiometer value
     if (VolCur == 0x8000) {  // If volume is neutral
       Volume = 0;  // Set volume to 0
     } else {
       Volume = VolCur * PotVal;  // Scale volume based on current values
     }
     val = VUM >> 20;  // Scale VUM for output
     VUM = 0;  // Reset VUM
     if (val > 7) val = 7;  // Limit value to max 7
   }

   LPC_TIM0->IR = 1;  // Clear the interrupt flag
}

void mp3Player() {

  uint32_t joystickInput;

	volatile uint32_t pclkdiv, pclk;

	SystemClockUpdate();  // Update system clock

	// Configure pins for MP3 interface
	LPC_PINCON->PINSEL1 &=~((0x03<<18)|(0x03<<20));  
	LPC_PINCON->PINSEL1 |= ((0x01<<18)|(0x02<<20));

	LPC_SC->PCONP |= (1 << 12);  // Power on the necessary peripherals

	// Configure ADC and DAC
	LPC_ADC->CR = 0x00200E04;		
	LPC_DAC->CR = 0x00008000;		

	// Setup PCLK for the timer
	pclkdiv = (LPC_SC->PCLKSEL0 >> 2) & 0x03;
	switch ( pclkdiv ) {
	case 0x00:
	default:
	pclk = SystemFrequency / 4;  // Default PCLK division
	break;
	case 0x01:
	pclk = SystemFrequency;  // No PCLK division
	break; 
	case 0x02:
	pclk = SystemFrequency / 2;  // Divide by 2
	break; 
	case 0x03:
	pclk = SystemFrequency / 8;  // Divide by 8
	break;
	}

	// Configure timer for data frequency
	LPC_TIM0->MR0 = pclk / DATA_FREQ - 1;	
	LPC_TIM0->MCR = 3;  // Enable timer interrupt and reset on match
	LPC_TIM0->TCR = 1;  // Start timer
	NVIC_EnableIRQ(TIMER0_IRQn);  // Enable interrupt for timer

	// Clear screen and display MP3 player info
    GLCD_Clear(White);
    GLCD_DisplayString(1, 0, __FI, "     MP3 Player      ");
	GLCD_Bitmap(160 - 100, 120 - 25, AUDIOMENU_WIDTH, AUDIOMENU_HEIGHT, AUDIOMENU_PIXEL_DATA);

    while (1) {

        joystickInput = get_button();  // Get joystick input
			
		USB_Init();  // Initialize USB
		USB_Connect(TRUE);  // Connect USB

        // If left button pressed, exit the MP3 player
        if (joystickInput == KBD_LEFT) {
            GLCD_Clear(White);
            return;
        }

    }

}

// Function to display the home menu using switch-case with a counter
void home_menu() {
    int counter = 0;  // 0: Slideshow, 1: Game, 2: Audio
    uint32_t joystickInput;
    GLCD_Clear(White);  // Set background to white
    GLCD_SetTextColor(Black);  // Set text color to black
    GLCD_SetBackColor(White);  // Set background color to white

    while (1) {
        GLCD_DisplayString(0, 0, __FI, "COE718 FINAL PROJECT");
        GLCD_DisplayString(1, 0, __FI, "     BY:  APATH     ");
        GLCD_DisplayString(2, 0, __FI, "    MEDIA CENTER    ");  
        GLCD_DisplayString(9, 0, __FI, " USE ^ and v to CTRL ");

        // Menu options (text and highlighted background for selected option)
        switch (counter) {
            case 0: // Slideshow selected
                GLCD_SetBackColor(Blue);  // Highlight Slideshow option
                GLCD_DisplayString(5, 0, __FI, "1) Slideshow------->");
                GLCD_SetBackColor(White);  // Reset background for the next option
                GLCD_DisplayString(6, 0, __FI, "2) Game             ");
                GLCD_DisplayString(7, 0, __FI, "3) MP3 Player       ");
                break;
            case 1: // Flash Black Screen selected
                GLCD_SetBackColor(Blue);  // Highlight Flash Black Screen option
                GLCD_DisplayString(6, 0, __FI, "2) Game------------>");
                GLCD_SetBackColor(White);  // Reset background for the next option
                GLCD_DisplayString(5, 0, __FI, "1) Slideshow        ");
                GLCD_DisplayString(7, 0, __FI, "3) MP3 Player       ");
                break;
            case 2: // MP3 Player selected
                GLCD_SetBackColor(Blue);  // Highlight MP3 Player option
                GLCD_DisplayString(7, 0, __FI, "3) MP3 Player------>");
                GLCD_SetBackColor(White);  // Reset background for the next option
                GLCD_DisplayString(5, 0, __FI, "1) Slideshow        ");
                GLCD_DisplayString(6, 0, __FI, "2) Game             ");
        }

        joystickInput = get_button();  // Get joystick input

        // Navigate down the menu
        if (joystickInput == KBD_DOWN) {
            counter++;  
            if (counter > 2) {
                counter = 0;  // Loop back to the first menu option
            }
            delay(500000);  // Debounce delay
        }

        // Navigate up the menu
        if (joystickInput == KBD_UP) {
            counter--;
            if (counter < 0) {
                counter = 2;  // Loop back to the last menu option
            }
            delay(500000);  // Debounce delay
        }

        // Select the highlighted option
        if (joystickInput == KBD_RIGHT) {  
            switch (counter) {
                case 0:
                    slideshow();  // Call slideshow function
                    break;
                case 1:
                    reactionTimeGame();  // Call flash black screen function
                    break;
                case 2:
                    mp3Player();  // Call MP3 player function
                    break;
            }
        }
    }
}

/*----------------------------------------------------------------------------
  Main Program
 *----------------------------------------------------------------------------*/
int main (void) {                       /* Main Program                       */
  LED_Init ();
  GLCD_Init();
	KBD_Init();
  GLCD_Clear  (White);

	while (1) {
		
		home_menu();
		
	}

}