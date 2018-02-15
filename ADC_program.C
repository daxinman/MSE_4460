#define  TASK_STK_SIZE		256	// Size of each task's stacks (# of bytes)
#define OS_MAX_EVENTS 		5
#define OS_MAX_TASKS 		15
#define OS_TASK_STAT_EN		1		// Enable statistics task creation
#define OS_TICKS_PER_SEC 	256		// Number of Ticks
#define ON 		1
#define OFF 	0
#use "ucos2.lib"
char DutyCycle;
char TaskData[3];						// Parameters to pass to each task
OS_EVENT		*DigOut;             // Define DigOut as an event

void   Task1(void *data);			// Function prototypes of task 1
void   Task2(void *data);			// Function prototypes of task 2
void	 Task3(void *data);

void   TaskStart(void *data);		// Function prototypes of Startup task
void   DispStr(int x, int y, char *s);	// Function prototypes of Display String function
void	 DisplayTower(void);

void main (void){
	brdInit();							// Initialize MCU baord
	OSInit();							// Initialize uC/OS-II
   DigOut = OSSemCreate(1);		// Semphore for accessing Digital Output

   OSTaskCreate(TaskStart, (void *)0, TASK_STK_SIZE, 10);
   OSStart();							// Start multitasking
}

void TaskStart (void *data){
    auto UBYTE err;
    auto WORD   key;
    static char   s[100];
    OSStatInit();

    OSTaskCreate(Task1, (void *)TaskData[0], TASK_STK_SIZE, 11);
    OSTaskCreate(Task2, (void *)TaskData[1], TASK_STK_SIZE, 12);
    OSTaskCreate(Task3, (void *)TaskData[2], TASK_STK_SIZE, 13);

		DispStr(0, 5, "#Tasks          : xxxxx  CPU Usage: xxxxx %");
		DispStr(0, 6, "#Task switch/sec: xxxxx  Duty Cycle: xx");
    DispStr(5, 10, "<-PRESS 'Q' or 'q' TO QUIT->");
    DisplayTower();

    for (;;) {
        sprintf(s, "%5d", OSTaskCtr);					// Display #tasks running
		  DispStr(18, 5, s);
    #if OS_TASK_STAT_EN
        sprintf(s, "%5d", OSCPUUsage);					// Display CPU usage in %
		  DispStr(36, 5, s);
    #endif
        sprintf(s, "%5d", OSCtxSwCtr);					// Display avg #context switches per 5 seconds
		  DispStr(18, 6, s);
        OSCtxSwCtr = 0;

        sprintf(s, "%2d", DutyCycle);					// Display avg #context switches per 5 seconds
		  DispStr(37, 6, s);
        OSCtxSwCtr = 0;

        if (kbhit()) {
            key = getchar();
				switch (key){
            	case 0x38:	// Increas duty cycle if "up" key
            		DutyCycle ++;
                  break;

            	case 0x32:	// Decrease duty cycle if "dn" key
            		DutyCycle --;
                  break;

            	case 0x71:  // Stop the code if it is 'q' key
                   exit(0);

               case 0x51: // Stop the code if it is 'Q' key
               	exit(0);
        		}
        }
        OSTimeDly(OS_TICKS_PER_SEC);					// Wait one second
    }
}

nodebug void Task1 (void *data){
	 auto UBYTE err;
    auto int output_level;
    output_level = ON;
    DutyCycle = 20;

    for (;;) {
        switch (output_level){    						 // toggle output_level
        	case ON:
         	output_level = OFF;
            break;
         case OFF:
         	output_level = ON;
            break;
         }
        OSSemPend(DigOut, 0, &err);						// Get the key to use DigOut function
    	  digOut(1, output_level);
        OSSemPost(DigOut);									// Release the Key for other tasks
        if (output_level == ON)
        	OSTimeDly(DutyCycle);
        else
        	OSTimeDly(32-DutyCycle);
        }
}

nodebug void Task2 (void *data)
{
	 auto UBYTE err;
    int output_level;

    output_level = OFF;
	for (;;) {
      output_level ^= ON; 									// toggle output_level  using XOR
      OSSemPend(DigOut, 0, &err);                  // Get the key to use DigOut function
    	digOut(2, output_level);
      OSSemPost(DigOut);  									// Release the Key for other tasks

      OSTimeDly(23);
    }
}

nodebug void Task3 (void *data){
	auto UBYTE err;
   int IrSensor;
	char ir[16];
   float x;
   int ym;

   for (;;) {
   	IrSensor = anaIn(1);
      DispStr(61,6+ym,"     ");

      x = ((IrSensor - 1650)*14) ;
      x = x/300;

   	if(x < 0){
      	ym = 0;
      } if (x > 14){
      	ym = 14;
      } else {
       	ym = x;
      }

      sprintf(ir,"%5d",IrSensor);
     	DispStr(61,6+ym,ir);
      OSTimeDly(30);
   }
}

void DispStr(int x, int y, char *s){
   x += 0x20;
   y += 0x20;
   printf ("\x1B=%c%c%s", x, y, s);
}

void DisplayTower (void){
	auto int i;
   const int XX, YY;
   XX = 60; YY = 5;
   DispStr (XX,YY,"=======");
   for(i=0;i<15;i++)
   	DispStr(XX,YY+1+i,"|     |");
   DispStr(XX,YY+1+i,"=======");
   DispStr(XX+1,YY+15,"XXXX");
}
