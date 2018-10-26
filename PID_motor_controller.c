/*  Definitions  */
#define  TASK_STK_SIZE		256         // Size of each task's stacks (# of bytes)
#define  PotMax				1031			// Maximum POT poistion reading
#define	PotMin				2088			// Minimum POT position reading
#define	IrVmax				1500			// Maximum IR Sesnor reading
#define	IrVmin				1990			// Minimum IR Sensor reading
#define  MAX_COUNT			100
#define	ON						1				// Both experimental, and development board work on an active low configuration
#define	OFF					0				// Both experimental, and development board work on an active low configuration
#define	N_TASKS				4				// Number of tasks
#define  IrMax					1650  //Lab1
#define  IrMin 				2050  //Lab1



// Redefine uC/OS-II configuration constants as necessary
#define  OS_MAX_EVENTS		2           // Maximum number of events (semaphores, queues, mailboxes)
#define  OS_MAX_TASKS		11          // Maximum number of tasks system can create (less stat and idle tasks)
#define  OS_TASK_STAT_EN	1           // Enable statistics task creation
#define  OS_TICKS_PER_SEC	128         // Number of Ticks per second

/* Other Definitions */
#define POT_CHAN				0                    // channel 0 of ADC (ADC0)
#define IRS_CHAN				1							// channel 1 of ADC (ADC1)
#define MOT_CHAN				1                    // channel 1 of digital output (OUT1)
#define MAX_PWIDTH			900                // the maximum pulse width in TMRB clock cycles

//#define STDIO_DISABLE_FLOATS

/* Variable declarations */
char	TMRB_MSB;									// this 8-bit value will be written to TBM2R (bits 8 and 9 of TMRB2 match register)
char	TMRB_LSB;									// this 8-bit value will be written to TBL2R (bits 0 to 7 of TMRB2 match register)
int	PulseWidth;                         // Duty Cycle of the PWM signal
float	PotNorm;										// Scaled value of the POT reading
int potNormInt;

int input_mode; //step 0  potentiometer 1
float ErrSig;
float PID_error;
int IrSen;
float IrNorm;
float desired;
float ballPosition;

float pastErrSig;
float Pgain;
float Igain;
float Dgain;

char TMRB_10_count;                       // The value is updated in the Timer B ISR to either 1 during the duty cycle or 0 for remainder.
#use "ucos2.lib"
UBYTE			TaskData[N_TASKS];      		// Parameters to pass to each task
OS_EVENT		*ADCSem;								// Semaphore to access ADC

void  InitializeTimers();						// Setup Timer A and B interrupts
void  CalculateDutyCycle();               // Update the duty cycle
void  Tmr_B_ISR();                        // Timer B interrupt service routine
void  ShowStat();                         // Display update
void  TaskInput(void *data);					// Function prototypes of the task
void	TaskControl (void *data);				// Function prototypes of the task
void  TaskStart(void *data);              // Function prototype of startup task
void  DispStr(int x, int y, char *s);


void main (void)
{
	Pgain = 0.8;   // default gain vales
   Dgain = 0.1;  // these are modified during the program operation
   Igain = 0.0;
   input_mode = 0;
   brdInit();                             // Initialize MCU baord
   OSInit();                              // Initialize uC/OS-II
   ADCSem = OSSemCreate(1);               // Semaphores for ADC inputs
   OSTaskCreate(TaskStart, (void *)0, TASK_STK_SIZE, 10);
   OSStart();                             // Start multitasking
}

void TaskStart (void *data)
{
   OSStatInit();
   OSTaskCreate(TaskInput, (void *)&TaskData[1], TASK_STK_SIZE, 11);
   OSTaskCreate(TaskControl, (void *)&TaskData[2], TASK_STK_SIZE, 5);
   InitializeTimers();
   DispStr(0, 10, "Analog Input Reading: xxxx  ");
   DispStr(0, 13, "#Tasks          : xxxxx  CPU Usage: xxxxx %");
   DispStr(0, 14, "#Task switch/sec: xxxxx");
   DispStr(0, 16, "Reference Point: ");
   DispStr(0, 17, "Ball Position: ");
   DispStr(0, 18, "Error Signal: ");
   DispStr(0, 19, "IR Norm: ");
   DispStr(0, 20, "IR Sen: ");
   DispStr(0, 21, "P GAIN: ");
   DispStr(0, 22, "I GAIN: ");
   DispStr(0, 23, "D GAIN: ");
   DispStr(0, 25, "Mode (0 step, 1 pot)");
   DispStr(0, 30, "<-PRESS 'Q' TO QUIT->");

   for (;;) {
         ShowStat();
         OSTimeDly(OS_TICKS_PER_SEC);     // Wait one second
    }
}


nodebug void TaskInput (void *date)
{
	auto	UBYTE err;
   char	display[64];
   int	PotRead;


   for(;;) {

   if(input_mode == 0){             //step input
     desired = 0.55;
   }

	   if(input_mode == 1){    //potentiometer input

		   OSSemPend(ADCSem,0,&err);
		   PotRead = anaIn(POT_CHAN);             // Read POT output voltage
		   OSSemPost(ADCSem);
		   PotNorm = (float)(PotRead - PotMax)/(float)(PotMin-PotMax);///(PotMin - PotMax);
		   sprintf(display, "%d", PotRead);       // Write POT voltage on STDIO
		   DispStr(22, 10, display);
	     desired = PotNorm;
	    }
     OSTimeDly(OS_TICKS_PER_SEC);
   }
}


nodebug void TaskControl (void *data)
{
   auto UBYTE err;

   for (;;) {

      OSSemPend(ADCSem,0,&err);
      IrSen = anaIn(IRS_CHAN);
      OSSemPost(ADCSem);

			// below is the piecewise linearization of the IR Sensor
			// the specific method is described in the project report
      if(IrSen > 2000){
	      IrNorm = 0;
      }

			else if(IrSen < 1500){
	      IrNorm = 1;
      }

			else{

      if(IrSen > 1950){

	      IrNorm = (float)(IrSen - 1950);
	      IrNorm = (float) (IrNorm / 50);
	      IrNorm = 1 - IrNorm;
	      IrNorm = IrNorm/2;
      }
      else{
	      IrNorm = (float)(IrSen - 1500);
	      IrNorm = (float)((IrNorm)/(500));
	      IrNorm = 1 - IrNorm;
      }

      }
      ballPosition = 44 * IrNorm;

			// PID calculation
			ErrSig = desired - IrNorm;
			PID_error = Pgain*(ErrSig) + Dgain*(ErrSig-pastErrSig)*0.5*(1/128) + Igain*(ErrSig+pastErrSig)*128;
      PulseWidth = (int)((0.7 + PID_error)*MAX_PWIDTH);
			// the above code is explained in the project report


      	if(PulseWidth < 0){
	         PulseWidth = 0;
	      }
	      if(PulseWidth > MAX_PWIDTH){
	         PulseWidth = MAX_PWIDTH;
				 }

    	TMRB_LSB = (char)(PulseWidth);
			TMRB_MSB = (char)(PulseWidth >> 2);
			// the above code ensures the duty cycle doesn't exceed its limits

		 pastErrSig = ErrSig; //holds the current error to be used
		                      //as the past error in the next function call
   OSTimeDly(1);
   }
}

nodebug root interrupt void Tmr_B_ISR()
{
   char TMRB_status;

   TMRB_status = RdPortI(TBCSR);					// Read-out and clear interrupt status flags
   if(TMRB_status & 0x02){							// A new PWM cycle, if Match 1 reg is triggered
      digOut(MOT_CHAN, ON);						// Set PWM output high
      WrPortI(TBM1R, NULL, 0x00);				// set up Match 1 reg to interrupt at the begining of the next cycle
      WrPortI(TBL1R, NULL, 0x00);				// set up Match 1 reg to interrupt at the begining of the next cycle
   }
   else if(TMRB_status & 0x04){					// If Match 2 reg is triggered, output will be low for the rest of the cycle
     digOut(MOT_CHAN, OFF);						// Set PWM output low */                                       // Drop output flag to 0
      WrPortI(TBM2R, NULL, TMRB_MSB);			// set up Match 2 reg to corespond with duty cycle
      WrPortI(TBL2R, NULL, TMRB_LSB);			// set up Match 2 reg to corespond with duty cycle
   }
   OSIntExit();
}


void InitializeTimers()
{
   TMRB_MSB = 0x40;									// Initialize TMRB2 match register to coincide with 50% duty cycle
   TMRB_LSB = 0xFF;									// Initialize TMRB2 match register to coincide with 50% duty cycle
   TMRB_10_count = 0;								// Initialize the Timer B interrupt counter (PWM cycle counter) to zero

   /* Setup Timer A */
   WrPortI(TAT1R, &TAT1RShadow, 0xFF);			// set TMRA1 to count down from 255
   WrPortI(TACR, &TACRShadow, 0x00);			// Disable TMRA interrupts (TMRA used only to clock TMRB)
   WrPortI(TACSR, &TACSRShadow, 0x01);			// Enable main clock (PCLK/2) for TMRA1

   /* set up Timer B for generating PWM */
   SetVectIntern(0x0b, Tmr_B_ISR);				// set up timer B interrupt vector
   WrPortI(TBCR, &TBCRShadow, 0x05);			// clock timer B with TMRA1 to priority 1 interrupt
   WrPortI(TBM1R, NULL, 0x00);					// set up match register 1 to 0x00
   WrPortI(TBL1R, NULL, 0x00);					// set up match register 1 to 0x00
   WrPortI(TBM2R, NULL, TMRB_MSB);				// set up match register 2 to to give an initial 50% duty cycle
   WrPortI(TBL2R, NULL, TMRB_LSB);				// set up match register 2 to to give an initial 50% duty cycle
   WrPortI(TBCSR, &TBCSRShadow, 0x07);			// enable Timer B to interrupt on B1 and B2 match
}

void ShowStat()
{
   static char Key, s[50];

   sprintf(s, "%5d", OSTaskCtr);					// Display #tasks running
   DispStr(18, 13, s);
   sprintf(s, "%5d", OSCPUUsage);				// Display CPU usage in %
   DispStr(36, 13, s);
   sprintf(s, "%5d", OSCtxSwCtr);				// Display avg #context switches per 5 seconds
   DispStr(18, 14, s);
   sprintf(s, "%.6f", PotNorm);
   DispStr(18, 10, s);
   sprintf(s, "%.6f",ballPosition);
   DispStr(18, 17, s);
   sprintf(s, "%.6f", ErrSig);
   DispStr(15, 18, s);
   sprintf(s, "%.6f", IrNorm);
   DispStr(15, 19, s);
   sprintf(s, "%5d", IrSen);
   DispStr(15, 20, s);
   sprintf(s, "%.6f", Pgain);
   DispStr(15, 21, s);
   sprintf(s, "%.6f", Igain);
   DispStr(15, 22, s);
   sprintf(s, "%.6f", Dgain);
   DispStr(15, 23, s);
   sprintf(s, "%d", input_mode);
   DispStr(25, 25, s);
   OSCtxSwCtr = 0;

   if (kbhit()) {										// See if key has been pressed
      Key = getchar();
      if (Key == 0x71 || Key == 0x51)			// Yes, see if it's the q or Q key
         exit(0);
      if (Key == 0x55) //p gain up
      	Pgain = Pgain + 0.1;
      if (Key == 0x59) //p gain down
      	Pgain = Pgain - 0.1;
      if (Key == 0x4A) //i gain up
      	Igain = Igain + 0.1;
      if (Key == 0x48) //i gain down
      	Igain = Igain - 0.1;
      if (Key == 0x4D) //d gain up
      	Dgain = Dgain + 0.1;
      if (Key == 0x4E) //d gain down
      	Dgain = Dgain - 0.1;
      if (Key == 0x5A) {
         input_mode = 0;
      if (Key == 0x58)
         input_mode = 1;
      }
   }
}

void DispStr (int x, int y, char *s)
{
   x += 0x20;
   y += 0x20;
   printf ("\x1B=%c%c%s", x, y, s);
}
