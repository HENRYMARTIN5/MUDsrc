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
// #include <ncurses.h>

#include <fcntl.h>
#include <sys/select.h>

#include <sys/stat.h>
#include <sys/types.h>

// #define clear() printf("\033[H\033[J")
#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

#define LOG_TIMESTAMPED(fmt, ...)                                                                                            \
    do                                                                                                                       \
    {                                                                                                                        \
        time_t t = time(NULL);                                                                                               \
        struct tm tm = *localtime(&t);                                                                                       \
        FILE *logF = fopen(logPath, "a");                                                                                    \
        if (logF != NULL)                                                                                                    \
        {                                                                                                                    \
            fprintf(logF, "%d:%02d:%02d [%s:%d] " fmt, tm.tm_hour, tm.tm_min, tm.tm_sec, __FILE__, __LINE__, ##__VA_ARGS__); \
            fclose(logF);                                                                                                    \
        }                                                                                                                    \
        printf("%d:%02d:%02d [%s:%d] " fmt, tm.tm_hour, tm.tm_min, tm.tm_sec, __FILE__, __LINE__, ##__VA_ARGS__);            \
    } while (0)

#define LOG_RAW(fmt, ...)                                                     \
    do                                                                        \
    {                                                                         \
        FILE *logF = fopen(logPath, "a");                                     \
        if (logF != NULL)                                                     \
        {                                                                     \
            fprintf(logF, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
            fclose(logF);                                                     \
        }                                                                     \
        printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);            \
    } while (0)

#define MASTER 0
#define PRESSURE_DEVICE 0
#define PRESSURE_CHANNEL 4
// device and channel for pressure
// #define DEVICE_COUNT 2

static void _mkdir(const char *dir)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

// device 1 channel 7 is pressure channel
// A tank is stainless steel
// B tank is PolyCarb
void configureChannels118();
void *dataFunction118();
void printBoard();
void AtoB();
void BtoA();
void stopFunc();
// void dataTrigger();
void changeAtoB(bool setting);
void changeBtoA(bool setting);
void changeAct(bool setting);
void changeData(bool setting);
void changeCam(bool setting);
void *regulatePressure();
void *updatePressure();
bool is_integer(const char *str);
char *pathCat();
void initializeGPIO();
void *systemLog(void *arg);

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
// const int fillACommand = 24;
// const int drainACommand = 7;
const int ABCommand = 1;
const int BACommmand = 5;
const int qCommand = 6;
pthread_t id;
pthread_t regPressureid;
time_t startPulse = -1;

const int R1 = 2;  // WHITE NOISE GEN
const int R2 = 3;  // S2
const int R3 = 4;  // S1
const int R4 = 14; // S3
const int R5 = 15; // pump
const int R6 = 18; // cam
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
// const int FLOWPIN = 3;//15
const int FLOWPIN = 21; // 40

int numDataFiles = 1;
int pump_speed;
int cumFlow = 0;
const int flowPulse = 20;
double pressure = 0;
double pressureVolts = 0;
const int OFF = 1;
const int ON = 0;
char *folder_path;
FILE **dataFile;
char logPath[40];
int DEVICE_COUNT = 2;
int channels_per_device = 8;
bool channels118[2][8];
//////////////////
void CCTrigger();
const uint8_t CC_pin = 24;
bool ccEvent;
bool s3_equalize;
bool quitting_program;

int main(int argc, char *argv[])
{
    ccEvent = false;
    quitting_program = false;
    s3_equalize = false;

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            channels118[i][j] = true;
        }
    }
    for (int i = 5; i < 8; i++)
    {
        channels118[0][i] = false;
    }
    for (int i = 5; i < 8; i++)
    {
        channels118[1][i] = false;
    }
    folder_path = pathCat();
    printf("Folder path: %s\n", folder_path);

    if (mkdir(folder_path, 0777))
    {
        printf("Unable to create folder\n");
        exit(1);
    }

    // make log file, put in folder
    strcpy(logPath, folder_path);
    strncat(logPath, "/log.csv", 9);
    // make data file, put in folder
    FILE *f;
    char path2[40];
    strcpy(path2, folder_path);
    strncat(path2, "/data1.csv", 11);
    f = fopen(path2, "w");
    if (f == NULL)
    {
        perror("Failed to open data file\n");
        exit(1);
    }
    dataFile = &f;

    LOG_TIMESTAMPED("Log Started\n");

    data = false;
    AB = false;
    BA = false;
    datalogger = false;
    pump_speed = 100;
    s1 = false;
    s2 = false;
    s3 = false;
    cam = false;
    initializeGPIO();

    LOG_TIMESTAMPED("GPIO initialized\n");

    pthread_t ptid_Log_Warning; // save all event kernel reports
    pthread_create(&ptid_Log_Warning, NULL, &systemLog, NULL);
    LOG_TIMESTAMPED("Kernel Log Started\n");

    digitalWrite(camera, ON);
    LOG_TIMESTAMPED("camera turned on\n");

    // changeAct(!act);
    digitalWrite(WHITE_NOISE_GEN, ON);
    LOG_TIMESTAMPED("act turned on\n");

    data = true;
    pthread_create(&id, NULL, dataFunction118, NULL);
    LOG_TIMESTAMPED("Data Collection Started\n");
    // wait until its almost cc
    while (!ccEvent)
    {
        usleep(10000);
    }

    LOG_TIMESTAMPED("CC\n");
    LOG_RAW("Experiment on\n"); // for debug purpose

    usleep(30000000); // Wait for Exp tank to settle 30 sec
    LOG_TIMESTAMPED("CC + 30\n");

    usleep(14500000); // wait 14.5 sec until liquid to equilibrate

    s3_equalize = true;
    pthread_create(&regPressureid, NULL, regulatePressure, NULL);
    LOG_TIMESTAMPED("Equalization Start\n");
    usleep(1500000); // wait 1.5 sec to avoid over current due to start up impedance

    AB = true; // Set A to B mode
    AtoB();
    LOG_TIMESTAMPED("A to B on\n");

    usleep(120000000); // 120 secs to drain////////////////////////////////////////////////////////////////////

    AB = false; // Set A to B mode off
    stopFunc();
    LOG_TIMESTAMPED("A to B off\n");

    usleep(1500000); // wait 1.5 sec to avoid over current due to start up impedance
    s3_equalize = false;
    pthread_join(regPressureid, NULL);
    LOG_TIMESTAMPED("Equalization End\n");

    // wait till decent
    //  Cameras need to be turned on again once after the last relavent recording
    //  This allows the recording to be saved, otherwise opening it will cause errors
    usleep(30000000);
    LOG_TIMESTAMPED("Experiment Off\n");

    ///////////////////////////////
    // camera video saving process
    digitalWrite(camera, HIGH); // turn off cameras
    usleep(5000000);
    digitalWrite(camera, LOW); // turn off cameras
    usleep(10000000);
    digitalWrite(camera, HIGH); // turn off cameras

    LOG_TIMESTAMPED("Quitting Program\n");
    data = false;
    quitting_program = true; // Signal to stop monitoring CC_pin

    digitalWrite(WHITE_NOISE_GEN, OFF); // turn off actuator

    // Wait for data collection to end
    pthread_join(id, NULL); // Wait for dataFunction118 to finish
    pthread_join(ptid_Log_Warning, NULL);
    LOG_TIMESTAMPED("Program ended safely\n");
    system("sudo shutdown now\n");
    return 0;
}

////////////////////
void AtoB()
{
    digitalWrite(pump, ON);
    digitalWrite(S1, OFF);
    digitalWrite(S2, ON);
    // digitalWrite(new_s3, ON);
}
void BtoA()
{
    digitalWrite(pump, ON);
    digitalWrite(S1, ON);
    digitalWrite(S2, OFF);
    // digitalWrite(new_s3, ON);
}

void *dataFunction118()
{
    // Create an array of addresses for each device
    uint8_t address[DEVICE_COUNT];
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
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
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        chan_mask[i] = 0x00 << 8;
    }

    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        for (int channel = 0; channel < 8; channel++)
        {
            if (channels118[device][channel])
            {
                totalChannels++;
                if (device == 0)
                {
                    device_1_chan_count++;
                }
                else
                {
                    device_2_chan_count++;
                }
                chan_mask[device] |= (0x01 << channel);
            }
        }
    }
    // finds max chan count between the hats
    uint8_t sample_cutter = device_1_chan_count ^ ((device_1_chan_count ^ device_2_chan_count) & -(device_1_chan_count < device_2_chan_count));
    ;
    // assert(device_2_chan_count == 4);
    double sample_rate_per_channel = ((100000 / sample_cutter)); // 100,000 max sample rate across all channels.

    int buffer_size = sample_rate_per_channel * 10;
    uint16_t status;
    uint32_t samples_read_per_chan[DEVICE_COUNT];
    uint32_t total_samples_read = 0;

    // Allocate memory for the buffer and sortbuffer
    double **buffer = (double **)malloc(DEVICE_COUNT * sizeof(double *));
    double **sortbuffer = (double **)malloc(totalChannels * sizeof(double *));
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        buffer[i] = (double *)malloc(buffer_size * 10 * sizeof(double));
        assert(buffer[i]);
    }
    for (int i = 0; i < totalChannels; i++)
    {
        sortbuffer[i] = (double *)malloc(buffer_size * 2 * sizeof(double));
        assert(sortbuffer[i]);
    }

    // Open the devices
    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        result = mcc118_open(address[device]);
        STOP_ON_ERROR(result);
    }

    // Check if the devices are open
    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        if (!mcc118_is_open(address[device]))
        {
            assert(0);
        }
    }

    // Print the sample rate

    // Start the scan
    uint32_t samples_per_channel = 100000;
    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        result = mcc118_a_in_scan_start(address[device], chan_mask[device], samples_per_channel, sample_rate_per_channel, OPTS_CONTINUOUS);
        STOP_ON_ERROR(result);
    }
    // fprintf(*logFile, "                DAQHat Sample rate:%f\n",sample_rate_per_channel);

    double actual_sample_rate = 0;
    mcc118_a_in_scan_actual_rate(sample_cutter, sample_rate_per_channel, &actual_sample_rate);
    LOG_RAW("DAQHat Sample rate:%f\n", actual_sample_rate);

    while (data)
    {
    } /////////////////////////////////////////////

    for (int i = 1; i < total_samples_read; i++)
    {
        for (int j = 0; j < totalChannels; j++)
        {
            fprintf(*dataFile, "%f, ", sortbuffer[j][i]);
        }
        fprintf(*dataFile, "\n");
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
    LOG_RAW("MCC cleared\n");

    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        free(buffer[device]);
    }

    LOG_RAW("buffer[device] freed\n");
    free(buffer);
    LOG_RAW("buffer freed\n");

    for (int chan = 0; chan < totalChannels; chan++)
    {
        free(sortbuffer[chan]);
    }
    LOG_RAW("sortbuffer[chan] freed\n");
    free(sortbuffer);
    LOG_RAW("sortbuffer freed\n");
}

char *pathCat()
{
    char *path = (char *)malloc(PATH_MAX);
    memset(path, 0, PATH_MAX); // reset all variables
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
    strncat(path, "./Blue-Data/results-", PATH_MAX);
    strncat(path, year, PATH_MAX);
    strncat(path, "-", PATH_MAX);
    strncat(path, month, PATH_MAX);
    strncat(path, "-", PATH_MAX);
    strncat(path, day, PATH_MAX);
    strncat(path, " ", PATH_MAX);
    strncat(path, hour, PATH_MAX);
    strncat(path, "_", PATH_MAX);
    strncat(path, min, PATH_MAX);
    strncat(path, "_", PATH_MAX);
    strncat(path, sec, PATH_MAX);
    // strncat(path,".csv",PATH_MAX);
    return path;
}

void initializeGPIO()
{
    wiringPiSetupGpio();

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

    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(R3, OUTPUT);
    pinMode(R4, OUTPUT);
    pinMode(R5, OUTPUT);
    pinMode(R6, OUTPUT);
    pinMode(R7, OUTPUT);
    pinMode(R8, OUTPUT);

    pinMode(CC_pin, INPUT);

    // wiringPiISR (dataCommand, INT_EDGE_RISING,  &dataTrigger);
    digitalWrite(R1, OFF);
    digitalWrite(R2, OFF);
    digitalWrite(R3, OFF);
    digitalWrite(R4, OFF);
    digitalWrite(R5, OFF);
    digitalWrite(R6, OFF);
    digitalWrite(R7, OFF);
    digitalWrite(R8, OFF);
    softPwmCreate(pwmPin, 0, 100);
    softPwmWrite(pwmPin, pump_speed);

    wiringPiISR(CC_pin, INT_EDGE_RISING, CCTrigger);
    pullUpDnControl(CC_pin, PUD_DOWN);
}

void stopFunc()
{
    digitalWrite(S1, OFF);
    digitalWrite(S2, OFF);
    digitalWrite(new_s3, OFF);
    digitalWrite(pump, OFF);
}

void *regulatePressure()
{ //.08948
    do
    {
        sleep(2);
        digitalWrite(new_s3, ON);
        usleep(400000);
        digitalWrite(new_s3, OFF);
    } while (s3_equalize);
}

////////////////////code for Blue flight

void *systemLog(void *arg)
{
    while (!quitting_program)
    {
        system("sudo dmesg -c >> ./Blue-Data/systemLog.csv\n"); // create or update systemLog.csv
        usleep(1000000);
    }
    pthread_exit(NULL);
    return 0;
}

void CCTrigger()
{
    usleep(100000);
    if (digitalRead(CC_pin) == HIGH)
    {
        ccEvent = true;
    }
}
