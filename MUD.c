#include "/home/pi/daqhats/examples/c/daqhats_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h> 
#include <linux/limits.h>
#include <wiringPi.h>
#include <ctype.h> 
#include <math.h>
#include <softPwm.h>
#include <assert.h>
//#include <ncurses.h>

#include <fcntl.h>
#include <sys/select.h>

#include <sys/stat.h>
#include <sys/types.h>

//#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))
#define MASTER 0
#define PRESSURE_DEVICE 0
#define PRESSURE_CHANNEL 4
 //device and channel for pressure
//#define DEVICE_COUNT 2

//device 1 channel 7 is pressure channel
//A tank is stainless steel
//B tank is PolyCarb
void configureChannels118();
void *dataFunction118();
void printBoard();
void AtoB();
void BtoA();
void stopFunc();
//void dataTrigger();
void changeAtoB(bool setting);
void changeBtoA(bool setting);
void changeAct(bool setting);
void changeData(bool setting);
void changeCam(bool setting);
void *regulatePressure();
void *updatePressure();
bool is_integer(const char *str);
char * pathCat();
void initializeGPIO();


bool act;
bool data;
bool AB;
bool BA;
bool datalogger;
bool cam;
bool s1;
bool s2;
bool s3;
bool high = false;
bool flightMode = false;
const int dataLED = 0;
const int dataCommand = 22;
//const int fillACommand = 24;
//const int drainACommand = 7;
const int ABCommand = 1;
const int BACommmand = 5;
const int qCommand = 6;
pthread_t id;
pthread_t regPressureid;
time_t startPulse = -1;

const int R1 = 2; //WHITE NOISE GEN
const int R2 = 3; //S2
const int R3 = 4; //S1
const int R4 = 14; //S3
const int R5 = 15; //pump
const int R6 = 18; //cam
const int R7 = 17;
const int R8 = 27;
const int S1 = R2;
const int S2 = R3;
const int WHITE_NOISE_GEN = R1;
const int S3 = R4;
const int pwmPin = 23;
const int pump = R5;
const int camera = R6;
const int new_s3 = R7;
//const int FLOWPIN = 3;//15
const int FLOWPIN = 21; //40
int numDataFiles = 1;
int pump_speed;
int cumFlow = 0;
const int flowPulse = 20;
double pressure = 0;
double pressureVolts = 0;
const int OFF = 1;
const int ON = 0;
char* folder_path; 
FILE ** logFile;
FILE ** dataFile;
char logPath[40];
int DEVICE_COUNT = 2;
int channels_per_device = 8;
bool channels118[2][8];



int main(){
    //set channels to false
    //initscr();
    for(int i = 0; i < 2; i++){
        for(int j = 0; j < 8; j++){
            channels118[i][j] = true;
        }
    }
    for(int i = 5; i < 8; i++){
        channels118[0][i] = false;
    }
    for(int i = 5; i < 8; i++){
        channels118[1][i] = false;
    }
    folder_path = pathCat();

    if (mkdir(folder_path, 0777)){
      printf("Unable to create folder\n");
      exit(1);
    }

    //make log file, put in folder
    FILE *log;
    logFile = &log;
    //char path[40];
    strcpy(logPath, folder_path);
    strncat(logPath,"/log.csv",9);
    //log = fopen(logPath,"w");
    //if(log == NULL){
    //    perror("Failed to open log file\n");
    //    exit(1);
    //}
    //make data file, put in folder
    FILE *f;
    char path2[40];
    strcpy(path2, folder_path);
    strncat(path2,"/data1.csv",11);
    f = fopen(path2,"w");
    if(f == NULL){
        perror("Failed to open data file\n");
        exit(1);
    }    
    dataFile = &f;



     
    //fprintf(*dataFile, "chan1, chan2, chan3, chan4, pressure, chan5, chan6, chan7, flowdata");
    //fprintf(*dataFile, "bingbong        Log Started\n");
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    *logFile = fopen(logPath,"a");
    if(log == NULL){
        perror("Failed to open log file\n");
        exit(1);
    }
    fprintf(*logFile, "%d:%d:%d        Log Started\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(*logFile);
    
    
    data = false;
    AB = false;
    BA = false;
    datalogger = false;
    pump_speed = 100;
    s1 =  false;
    s2 = false;
    s3 = false;
    cam = false;
    initializeGPIO();

    


   int wordCount = 0;
   char commands[20][20]; //max of 20 commands per line, each command has max of 20 chars
   bool q = true;
   while(q){
       wordCount = 0;
       memset(commands, 0, sizeof(commands));
       printf("\033[H\033[J");
       printBoard();


       while (wordCount < 20 && scanf("%s", commands[wordCount]) == 1) {
            //printf("\033[H\033[J");
            //printf("\n\n\nbingbong:::: %s\n\n", commands[wordCount]);
            //assert(false);

            // Convert input to lowercase
            for (int i = 0; commands[wordCount][i]; i++) {
                commands[wordCount][i] = tolower((unsigned char)commands[wordCount][i]);
            }


           if(strlen(commands[wordCount]) >= 2){
               if(commands[wordCount][0] == 'd' & commands[wordCount][1] == 't'){//user inputted a draintest command
                  
                   if(strlen(commands[wordCount]) > 2){//there are numbers after it, change speed value
                       char strspeed[4];
                       strcpy(strspeed, commands[wordCount]+2); //isolate the numbers on the end of the command
                      
                       char *endptr; 
                       long num = strtol(strspeed, &endptr, 10); //convert char[] to int. (base10)
                      
                       //assert(false);
                       // Check for conversion errors
                      
                       time_t t = time(NULL);
                       struct tm tm = *localtime(&t);
                        log = fopen(logPath,"w");
                        if(log == NULL){
                            perror("Failed to open log file\n");
                                exit(1);
                        }
                       fprintf(*logFile, "%d:%d:%d        pump speed changed from: %d\n",tm.tm_hour, tm.tm_min, tm.tm_sec, pump_speed);
                       fprintf(*logFile, " to: %d\n",num);
                       fclose(*logFile);
                       softPwmWrite (pwmPin, num);
                       pump_speed = num;
                   }
                   //replace draintest command with the following
                   if(data){
                       if(!flightMode){
                           strcpy(commands[wordCount++], "data");   // Replaces 'draintestxxx' with 'act' and increments wordCount
                       }
                       strcpy(commands[wordCount++], "atob");  // Adds 'atob' after 'act' and increments wordCount
                       strcpy(commands[wordCount], "act");
                   }else{
                       strcpy(commands[wordCount++], "act");   // Replaces 'draintestxxx' with 'act' and increments wordCount
                       strcpy(commands[wordCount++], "atob");  // Adds 'atob' after 'act' and increments wordCount
                       if(!flightMode){
                           strcpy(commands[wordCount], "data");
                       }else{
                           wordCount -= 1;
                       }
                   }
               }
              
              
           }




          
           wordCount++;
           char c = getchar(); // Read the next character
           //assert(false);
           if (c == '\n') {
               break; // Stop reading if newline is encountered
           }
       }  


      
       // for(int i =0; i < strlen(command); i++){
       //     command[i] = tolower(command[i]);
       // }
       *logFile = fopen(logPath,"a");
        if(log == NULL){
            perror("Failed to open log file\n");
            exit(1);
        }
       for(int commandCount = 0; commandCount < wordCount; commandCount++){ //loop command list for each command
           if(strcmp(commands[commandCount],"data") == 0){
               changeData(!data);
               if(data){
                   pthread_create(&id, NULL, updatePressure, NULL);
                   pthread_create(&id, NULL, dataFunction118, NULL);
               }else{
                   pthread_join(id, NULL);
               }
           }else if(strcmp(commands[commandCount],"flight") == 0){
               flightMode = !flightMode;
           }else if(strcmp(commands[commandCount],"s1") == 0){
               s1 = !s1;
                       if(s1){
                   digitalWrite(S1, ON);
               }else{
                   digitalWrite(S1, OFF);
               }
           }else if(strcmp(commands[commandCount],"s2") == 0){
               s2 = !s2;
                       if(s2){
                   digitalWrite(S2, ON);
               }else{
                   digitalWrite(S2, OFF);
               }
           }else if(strcmp(commands[commandCount],"s3") == 0){
              time_t t = time(NULL);
              struct tm tm = *localtime(&t);
               s3 = !s3;
               if(s3){
                  digitalWrite(new_s3, ON);
                  fprintf(*logFile, "%d:%d:%d        S3 turned on\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
               }else{
                  digitalWrite(new_s3, OFF);
                  fprintf(*logFile, "%d:%d:%d        S3 turned off\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
               }
           }else if(strcmp(commands[commandCount],"file") == 0){
               FILE *f;
               numDataFiles++;
               char* numDataFilesStr = (char*)malloc(3);
               snprintf(numDataFilesStr, 3, "%d", numDataFiles); //The threes are for a max of 2 digits and a null terminator


               char path[40];
               strcpy(path, folder_path);
               strncat(path,"/data",6);
               strncat(path, numDataFilesStr, 3);
               strncat(path, ".csv", 5);
               f = fopen(path,"w");
               if(f == NULL){
                   perror("Failed to open file\n");
                   exit(1);
               }   
              
               dataFile = &f;
               free(numDataFilesStr);
               fprintf(*dataFile, "chan1, chan2, chan3, chan4, pressure, chan5, chan6, chan7, flowdata");
           }else if(strcmp(commands[commandCount],"atob") == 0){
               changeBtoA(false);
               changeAtoB(!AB);
               if(AB){
                   
                   AtoB();
               }else{
                   stopFunc();
               }
              
           }else if(strcmp(commands[commandCount],"btoa") == 0){
               changeAtoB(false);
               changeBtoA(!BA);
               if(BA){
                   BtoA();
               }else{
                   stopFunc();
               }
           }else if(strcmp(commands[commandCount],"act") == 0){
               changeAct(!act);
               if(act){
                   digitalWrite(WHITE_NOISE_GEN, ON);
               }else{
                   digitalWrite(WHITE_NOISE_GEN, OFF);
               }
           }else if(strcmp(commands[commandCount],"q") == 0){
               printf("\033[H\033[J");
               changeAtoB(false);
               changeBtoA(false);
               changeAct(false);
               changeData(false);
               digitalWrite(R1, OFF);
               digitalWrite(R2, OFF);
               digitalWrite(R3, OFF);
               digitalWrite(R4, OFF);
               digitalWrite(R5, OFF);
               digitalWrite(R6, OFF);
               digitalWrite(R7, OFF);
               digitalWrite(R8, OFF);
               softPwmWrite (pwmPin, 0);
               pthread_join(id,NULL);
               usleep(10000); 
               q = false;
               break;
           }else if(strcmp(commands[commandCount],"speed") == 0){
               printf("\033[H\033[J");
               printf("enter the pump speed you want 0-100:");
               time_t t = time(NULL);
               struct tm tm = *localtime(&t);
               fprintf(*logFile, "%d:%d:%d        pump speed changed from: %d\n",tm.tm_hour, tm.tm_min, tm.tm_sec, pump_speed);
               scanf("%d",&pump_speed);
               fprintf(*logFile, " to: %d\n",pump_speed);
               softPwmWrite (pwmPin, pump_speed);
               usleep(10000); 
           }else if(strcmp(commands[commandCount],"count") == 0){
               printf("\033[H\033[J");
               int c;
               printf("Enter Device Count\n");
               if(scanf("%d", &c) == 1){
                   if(c == 1){
                       DEVICE_COUNT = 1;
                   }else if(c == 2){
                       DEVICE_COUNT = 2;
                   }
                   time_t t = time(NULL);
                   struct tm tm = *localtime(&t);
                   fprintf(*logFile, "%d:%d:%d        device count change to: %d\n",tm.tm_hour, tm.tm_min, tm.tm_sec, DEVICE_COUNT);
               }
               usleep(10000);
           }else if(strcmp(commands[commandCount],"channels") == 0){
               printf("\033[H\033[J");
               configureChannels118();
           }else if(strcmp(commands[commandCount],"cam") == 0){
               changeCam(!cam);
               if(cam){
                   digitalWrite(camera,ON);
               }else{
                   digitalWrite(camera,OFF);
               }
           }
           
       }
       fclose(*logFile);
   }
   //endwin();
   return 0;
}
void* regulatePressure(){ //.08948
       while(AB | BA){
           sleep(2);
           digitalWrite(new_s3, ON);
           usleep(400000);
           digitalWrite(new_s3, OFF);
       }
}
void* updatePressure(){ //(voltage-1) *44.7/4
   while(data) {
       pressure = ((pressureVolts - 1.0) * (44.7/4));
       printf("\033[s"); //saves current data position
       printf("\033[%d;%dH", 9, 18); // move cursor to x=11, y=18
       printf("%f               \n", pressure); //rewrites pressure
       printf("\033[u"); //restores saved cursor position
       fflush(stdout); // ensure output is immediately printed to the terminal
       sleep(1);
   }
}
void configureChannels118(){
   for(int device = 0; device < DEVICE_COUNT; device++){
       printf("\033[H\033[J");
       printf("type the channel number to toggle channel. Type 9 to continue\n");
       printf("DEVICE %d:\n___________\n", device+1);
       for(int channel = 0; channel < 8; channel++){
           if(channels118[device][channel]){
               printf("channel %d: ON\n", channel);
           }else{
               printf("channel %d: OFF\n", channel);
           }
       }
       int num;
       int result;
       do {
           result = scanf("%d", &num);
           if (result == 0) {
               printf("Error: Invalid input. Please enter a number.\n");
               scanf("%*[^\n]"); // clear input buffer
           }
       } while (result == 0);
       if(num >= 0 && num <= 7){
           channels118[device][num] = !channels118[device][num];
           device--;
       }
      
      
   }
}




void stopFunc(){
   digitalWrite(S1, OFF);
   digitalWrite(S2, OFF);
   digitalWrite(new_s3, OFF);
   digitalWrite(pump, OFF);
}


void printBoard(){
   printf("\033[H\033[J");
   printf("enter a command\n"
   "Commands: atob, btoa, act, data, speed, cam, count, channels, file, q\n""\n");
   printf("Actuator is %s\n", act ? "on" : "off");
   printf("A to B is %s\n", AB ? "on" : "off");
   printf("B to A is %s\n", BA ? "on" : "off");
   printf("data collection is %s\n", data ? "on" : "off");
   printf("Camera is %s\n", cam ? "on" : "off");
   printf("Current pressure:%f\n",pressure);
   printf("Current Pump Speed:%d\n",pump_speed);
   printf("Current Device Count: %d\n", DEVICE_COUNT);
   printf("Flight Mode is %s", flightMode ? "on" : "off");


  
   gotoxy(0,3);
}


void *dataFunction118(){
   // Create an array of addresses for each device
   uint8_t address[DEVICE_COUNT];
   for(int i = 0; i < DEVICE_COUNT; i++){
       address[i] = i;
   }
  
   // Initialize variables
   int result = 0;
   uint8_t synced;
   uint8_t clock_source;
   int totalChannels = 0;
   int device_1_chan_count = 0;
   int device_2_chan_count = 0;
   uint16_t chan_mask[DEVICE_COUNT];
   for(int i = 0; i < DEVICE_COUNT; i++){
       chan_mask[i] = 0x00 << 8;
   }
  
   for(int device = 0; device < DEVICE_COUNT; device++){
       for(int channel = 0; channel < 8; channel++){
           if(channels118[device][channel]){
               totalChannels++;
               if(device == 0){
                   device_1_chan_count++;
               }else{
                   device_2_chan_count++;
               }
               chan_mask[device] |= (0x01 << channel);
           }
       }
   }
   //finds max chan count between the hats
   uint8_t sample_cutter = device_1_chan_count ^ ((device_1_chan_count ^ device_2_chan_count) & -(device_1_chan_count < device_2_chan_count));;
   //assert(device_2_chan_count == 4);
   double sample_rate_per_channel = ((100000/sample_cutter)); //100,000 max sample rate across all channels.
    printf("                DAQHat Sample rate:%f\n", sample_rate_per_channel); // for debug purpose

  


   int buffer_size = sample_rate_per_channel * 10;
   uint16_t status;
   uint32_t samples_read_per_chan[DEVICE_COUNT];
   uint32_t total_samples_read = 0;


   // Allocate memory for the buffer and sortbuffer
   double ** buffer = (double **)malloc(DEVICE_COUNT * sizeof(double*));
   double ** sortbuffer = (double **)malloc(totalChannels * sizeof(double*));
   for (int i = 0; i < DEVICE_COUNT; i++) {
       buffer[i] = (double *)malloc(buffer_size * 10 * sizeof(double));
       assert(buffer[i]);
   }
   for (int i = 0; i < totalChannels; i++) {
       sortbuffer[i] = (double *)malloc(buffer_size * 2 * sizeof(double));
       assert(sortbuffer[i]);
   }


   // Open the devices
   for(int device = 0; device < DEVICE_COUNT; device++){
       result = mcc118_open(address[device]);
       STOP_ON_ERROR(result);
   }
  
   // Check if the devices are open
   for(int device = 0 ; device < DEVICE_COUNT; device ++){
       if(!mcc118_is_open(address[device])){
           assert(0);
       }
   }
  
   // Print the sample rate
  


   // Start the scan
   uint32_t samples_per_channel = 100000;
   for(int device = 0; device < DEVICE_COUNT;device++){
       result = mcc118_a_in_scan_start(address[device], chan_mask[device], samples_per_channel, sample_rate_per_channel, OPTS_CONTINUOUS);
       STOP_ON_ERROR(result);
   }
   //fprintf(*logFile, "                DAQHat Sample rate:%f\n",sample_rate_per_channel);


   double actual_sample_rate = 0;
   mcc118_a_in_scan_actual_rate(sample_cutter, sample_rate_per_channel, &actual_sample_rate);
   *logFile = fopen(logPath,"a");
    if(log == NULL){
        perror("Failed to open log file\n");
        exit(1);
    }
   fprintf(*logFile, "               DAQHat Sample rate:%f\n",actual_sample_rate);
   fclose(*logFile);


   while(data){  
      
       usleep(500);     
       for(int device = 0; device < DEVICE_COUNT; device++){           
           mcc118_a_in_scan_read(address[device], &status, -1, 0, buffer[device], buffer_size, &samples_read_per_chan[device]);
           if(status & STATUS_HW_OVERRUN || status & STATUS_BUFFER_OVERRUN){
               if(status & STATUS_HW_OVERRUN){
                   printf("Hardware OverRun\n");
               }else{
                   printf("Buffer OverRun\n");
               }
               goto stop;
           }
       }
       //printf("%d\n",samples_read_per_chan[0]);
       //device 1 sort
       int index = total_samples_read;
      
       for(int i = 0; i < samples_read_per_chan[0]*device_1_chan_count; i++){
               sortbuffer[(i % device_1_chan_count)][index] = buffer[0][i];
               if(i % device_1_chan_count == 0){
                   index++;
               }
       }// usleep(100000) usleep(1000000)
       //device 2 sort
       if(device_2_chan_count > 0){
           int index = total_samples_read;
           for(int i = 0; i < samples_read_per_chan[0]*device_2_chan_count; i++){
                   sortbuffer[(device_1_chan_count) + (i % device_2_chan_count)][index] = buffer[1][i];
                   if(i % device_2_chan_count == 0){
                       index++;
                   }
           }
       }
      
       total_samples_read += samples_read_per_chan[0];
       if(channels118[PRESSURE_DEVICE][PRESSURE_CHANNEL]){
           pressureVolts = sortbuffer[device_1_chan_count - 1][total_samples_read];
       }
       if(total_samples_read >= 10000){      //if the 10,000 value is too big, it takes too long to print to file. bufferOverrun
           //printf("%d\n",total_samples_read);
           for(int i =1; i < total_samples_read; i++){
               for(int j = 0; j < totalChannels; j++){
                   fprintf(*dataFile, "%f, ", sortbuffer[j][i]);
               }  
               fprintf(*dataFile,"\n");           
           }
           total_samples_read = 0;
       }       
  }
 
   for(int i =1; i < total_samples_read; i++){
       for(int j = 0; j < totalChannels; j++){
           fprintf(*dataFile, "%f, ", sortbuffer[j][i]);
       }  
       fprintf(*dataFile,"\n");           
   }
  
stop:
   for (int device = 0; device < DEVICE_COUNT; device++)
   {
       result = mcc118_a_in_scan_stop(address[device]);
       print_error(result);
      
       result = mcc118_a_in_scan_cleanup(address[device]);
       print_error(result);


       result = mcc118_close(address[device]);
       print_error(result);
   }


   for(int device =0; device < DEVICE_COUNT; device++){
       free(buffer[device]);
   }
   free(buffer);
   for(int chan =0; chan < totalChannels; chan++){
       free(sortbuffer[chan]);
   }
   free(sortbuffer);
}


//transferring stainless steel to polycarb tank
//4 GPIO pump, S1, S2, S3
//pump is a pwmwrite or sum gen/d by pi
void AtoB(){
   digitalWrite(pump,ON);
   digitalWrite(S1,OFF);
   digitalWrite(S2, ON);
   digitalWrite(new_s3,ON);
}




void BtoA(){
   digitalWrite(pump,ON);
   digitalWrite(S1,ON);
   digitalWrite(S2, OFF);
   digitalWrite(new_s3,ON);
}


void changeAct(bool setting){
   if(setting != act){
       act = setting;
       time_t t = time(NULL);
       struct tm tm = *localtime(&t);
       if(setting){
           fprintf(*logFile, "%d:%d:%d        actuator turned on\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }else{
           fprintf(*logFile, "%d:%d:%d        actuator turned off\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }
   }
}
void changeAtoB(bool setting){
   if(setting != AB){
      
       AB = setting;
       time_t t = time(NULL);
       struct tm tm = *localtime(&t);
       if(setting){
            //pthread_create(&regPressureid, NULL, regulatePressure, NULL);
            fprintf(*logFile, "%d:%d:%d        AtoB turned on\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }else{
            //pthread_join(regPressureid, NULL);
            fprintf(*logFile, "%d:%d:%d        AtoB turned off\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }
   }
}
void changeBtoA(bool setting){
   if(setting != BA){
       BA = setting;
       time_t t = time(NULL);
       struct tm tm = *localtime(&t);
       if(setting){
            //pthread_create(&regPressureid, NULL, regulatePressure, NULL);
            fprintf(*logFile, "%d:%d:%d        BtoA turned on\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }else{
            //pthread_join(regPressureid, NULL);
            fprintf(*logFile, "%d:%d:%d        BtoA turned off\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }
   }
}
void changeData(bool setting){
   if(setting != data){
       data = setting;
       time_t t = time(NULL);
       struct tm tm = *localtime(&t);
       if(setting){
           fprintf(*logFile, "%d:%d:%d        data turned on\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }else{
           fprintf(*logFile, "%d:%d:%d        data turned off\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }
   }
}
void changeCam(bool setting){
   if(setting != cam){
       cam = setting;
       time_t t = time(NULL);
       struct tm tm = *localtime(&t);
       if(setting){
           fprintf(*logFile, "%d:%d:%d        cam turned on\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }else{
           fprintf(*logFile, "%d:%d:%d        cam turned off\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
       }
   }
}
char * pathCat(){
   char * path = (char *)malloc(PATH_MAX);
   memset(path,0,PATH_MAX); //reset all variables
   time_t t = time(NULL);
   srand(time(NULL));
   struct tm tm = *localtime(&t);
   char year[10];
   sprintf(year, "%d", tm.tm_year + 1900);
   char month[10];
   sprintf(month, "%d", tm.tm_mon + 1);
   char day[10];
   sprintf(day, "%d", tm.tm_mday);
   char hour[10];
   sprintf(hour, "%d", tm.tm_hour);
   char min[10];
   sprintf(min, "%d", tm.tm_min);
   char sec[10];
   sprintf(sec, "%d", tm.tm_sec);
   strncat(path,"./Data/results-",PATH_MAX);
   strncat(path,year,PATH_MAX);
   strncat(path,"-",PATH_MAX);
   strncat(path,month,PATH_MAX);
   strncat(path,"-",PATH_MAX);
   strncat(path,day,PATH_MAX);
   strncat(path," ",PATH_MAX);
   strncat(path,hour,PATH_MAX);
   strncat(path,"꞉",PATH_MAX);
   strncat(path,min,PATH_MAX);
   strncat(path,"꞉",PATH_MAX);
   strncat(path,sec,PATH_MAX);
   //strncat(path,".csv",PATH_MAX);
   return path;
  
}

void initializeGPIO(){
    wiringPiSetupGpio ();
    
    // pinMode (0, OUTPUT);
    // pinMode (1, OUTPUT);
    // pinMode (2, OUTPUT);
    // pinMode (3, OUTPUT);
    // pinMode (4, OUTPUT);
    // pinMode (5, OUTPUT);
    // pinMode (6, OUTPUT);
    // pinMode (7, OUTPUT);
    // pinMode (8, OUTPUT);
    // pinMode (9, OUTPUT);
    // pinMode (10, OUTPUT);
    // pinMode (11, OUTPUT);
    // pinMode (12, OUTPUT);
    // pinMode (13, OUTPUT);
    // pinMode (14, OUTPUT);
    // pinMode (15, OUTPUT);
    // pinMode (16, OUTPUT);
    // pinMode (17, OUTPUT);
    // pinMode (18, OUTPUT);
    // pinMode (19, OUTPUT);
    // pinMode (20, OUTPUT);
    // pinMode (21, OUTPUT);
    // pinMode (22, OUTPUT);
    // pinMode (23, OUTPUT);
    // pinMode (24, OUTPUT);
    // pinMode (25, OUTPUT);
    // pinMode (26, OUTPUT);
    // pinMode (27, OUTPUT);
    // pinMode (28, OUTPUT);
    // pinMode (29, OUTPUT);
    // pinMode (30, OUTPUT);
    // pinMode (31, OUTPUT);

    // pinMode (0, OFF);
    // pinMode (1, OFF);
    // pinMode (2, OFF);
    // pinMode (3, OFF);
    // pinMode (4, OFF);
    // pinMode (5, OFF);
    // pinMode (6, OFF);
    // pinMode (7, OFF);
    // pinMode (8, OFF);
    // pinMode (9, OFF);
    // pinMode (10, OFF);
    // pinMode (11, OFF);
    // pinMode (12, OFF);
    // pinMode (13, OFF);
    // pinMode (14, OFF);
    // pinMode (15, OFF);
    // pinMode (16, OFF);
    // pinMode (17, OFF);
    // pinMode (18, OFF);
    // pinMode (19, OFF);
    // pinMode (20, OFF);
    // pinMode (21, OFF);
    // pinMode (22, OFF);
    // pinMode (23, OFF);
    // pinMode (24, OFF);
    // pinMode (25, OFF);
    // pinMode (26, OFF);
    // pinMode (27, OFF);
    // pinMode (28, OFF);
    // pinMode (29, OFF);
    // pinMode (30, OFF);
    // pinMode (31, OFF);

    pinMode (R1, OUTPUT);
    pinMode (R2, OUTPUT);
    pinMode (R3, OUTPUT);
    pinMode (R4, OUTPUT);
    pinMode (R5, OUTPUT);
    pinMode (R6, OUTPUT);
    pinMode (R7, OUTPUT);
    pinMode (R8, OUTPUT);
    
    //wiringPiISR (dataCommand, INT_EDGE_RISING,  &dataTrigger);
    digitalWrite(R1, OFF);
    digitalWrite(R2, OFF); 
    digitalWrite(R3, OFF);
    digitalWrite(R4, OFF);
    digitalWrite(R5, OFF);
    digitalWrite(R6, OFF);
    digitalWrite(R7, OFF);
    digitalWrite(R8, OFF);
    softPwmCreate(pwmPin, 0, 100);
    softPwmWrite (pwmPin, pump_speed);
}
