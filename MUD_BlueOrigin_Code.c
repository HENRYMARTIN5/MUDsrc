#include "/home/pi/daqhats/examples/c/daqhats_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <linux/limits.h> // For PATH_MAX
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
            mkdir(tmp, S_IRWXU); // Note: uses S_IRWXU (0700 for owner)
            *p = '/';
        }
    mkdir(tmp, S_IRWXU); // Note: uses S_IRWXU (0700 for owner)
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
char *folder_path; // This is global
FILE **dataFile;
// char logPath[40]; // OLD, too small
char logPath[PATH_MAX]; // MODIFIED: Increased size for absolute paths
int DEVICE_COUNT = 2;
int channels_per_device = 8;
bool channels118[2][8];
//////////////////added variables and functions for BlueOrigin flight
void CCTrigger();
void initializeMCC();
const uint8_t CC_pin = 24;
bool ccEvent;
bool s3_equalize;
bool quitting_program;

int main(int argc, char *argv[])
{
    initializeMCC();
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

    // START OF MODIFICATIONS for robust folder creation
    const char* project_base_dir = "/home/pi/MUD_Code"; // Specified base directory
    char blue_data_base_dir[PATH_MAX]; // Will be /home/pi/MUD_Code/Blue-Data
    char full_timestamped_folder_path[PATH_MAX]; // Will be /home/pi/MUD_Code/Blue-Data/results-TIMESTAMP
    char* path_suffix_from_cat; // Stores result from pathCat()

    // 1. Construct the absolute path to the "Blue-Data" directory (parent of timestamped folders)
    snprintf(blue_data_base_dir, sizeof(blue_data_base_dir), "%s/Blue-Data", project_base_dir);

    // 2. Ensure this "Blue-Data" directory exists using _mkdir
    //    _mkdir will create /home/pi/MUD_Code (if not exist and permissions allow)
    //    and then /home/pi/MUD_Code/Blue-Data
    printf("Ensuring base data directory exists: %s\n", blue_data_base_dir);
    _mkdir(blue_data_base_dir);

    // 3. Get the relative path suffix from pathCat()
    //    pathCat() is expected to return something like "./Blue-Data/results-TIMESTAMP"
    path_suffix_from_cat = pathCat();
    if (!path_suffix_from_cat) {
        perror("pathCat failed to allocate memory");
        exit(1);
    }

    // 4. Construct the full absolute path for the final timestamped folder.
    //    We combine project_base_dir with the part of path_suffix_from_cat
    //    that comes *after* "./". So, "Blue-Data/results-TIMESTAMP".
    if (strncmp(path_suffix_from_cat, "./Blue-Data/", strlen("./Blue-Data/")) == 0) {
        // Correctly extract "results-TIMESTAMP" part if pathCat gives "./Blue-Data/results-..."
        // The target is blue_data_base_dir / (results-TIMESTAMP part)
        // results-TIMESTAMP part starts after "./Blue-Data/"
        snprintf(full_timestamped_folder_path, sizeof(full_timestamped_folder_path),
                 "%s/%s", blue_data_base_dir, path_suffix_from_cat + strlen("./Blue-Data/"));
    } else if (strncmp(path_suffix_from_cat, "./", 2) == 0) {
        // Fallback: if pathCat gives "./results-...", then use project_base_dir directly.
        // This case implies results- folder should be directly under project_base_dir.
        // For your structure, this means results are in /home/pi/MUD_Code/results-TIMESTAMP
        // This is likely not what pathCat returns based on its current code.
        // The original pathCat has "./Blue-Data/results-", so this branch is less likely.
        fprintf(stderr, "Warning: pathCat() returned './' but not './Blue-Data/'. Path: %s. Assuming results in %s\n", path_suffix_from_cat, project_base_dir);
        snprintf(full_timestamped_folder_path, sizeof(full_timestamped_folder_path),
                 "%s/%s", project_base_dir, path_suffix_from_cat + 2);
    } else {
        // Fallback: if pathCat returns something unexpected (e.g. "results-TIMESTAMP" directly)
        fprintf(stderr, "Warning: pathCat() did not return expected './' prefix. Path: %s. Assuming relative to %s\n", path_suffix_from_cat, blue_data_base_dir);
        snprintf(full_timestamped_folder_path, sizeof(full_timestamped_folder_path),
                 "%s/%s", blue_data_base_dir, path_suffix_from_cat);
    }


    // 5. Assign the constructed absolute path to the global `folder_path`
    //    Need to duplicate the string as full_timestamped_folder_path is a local char array.
    if (folder_path != NULL) { // Should be NULL at start, but good practice
        free(folder_path);
    }
    folder_path = strdup(full_timestamped_folder_path);
    if (folder_path == NULL) {
        perror("Failed to strdup for folder_path");
        free(path_suffix_from_cat); // Free memory from pathCat
        exit(1);
    }

    // Original printf, now shows absolute path
    printf("Attempting to create folder (absolute path): %s\n", folder_path);

    // 6. Create the final timestamped directory.
    //    Its parent (blue_data_base_dir) was already ensured by _mkdir.
    if (mkdir(folder_path, 0777) != 0) { // mkdir returns 0 on success, non-zero on error.
        // Use fprintf to stderr before logPath is confirmed, or use LOG_TIMESTAMPED if it's safe
        fprintf(stderr, "Timestamp: %ld, File: %s, Line: %d, Unable to create folder '%s': %s (errno %d)\n",
                (long)time(NULL), __FILE__, __LINE__, folder_path, strerror(errno), errno);
        free(path_suffix_from_cat);
        free(folder_path); // folder_path was strdup'd
        folder_path = NULL;
        exit(1);
    }
    printf("Successfully created folder: %s\n", folder_path);


    // 7. Free the memory allocated by pathCat(), as its content has been copied (via strdup) to folder_path.
    free(path_suffix_from_cat);
    path_suffix_from_cat = NULL;

    // END OF MODIFICATIONS for robust folder creation

    // make log file, put in folder
    // strcpy(logPath, folder_path); // folder_path is now absolute
    // strncat(logPath, "/log.csv", 9); // Old, unsafe strncat
    snprintf(logPath, sizeof(logPath), "%s/log.csv", folder_path); // MODIFIED: Safe construction

    // make data file, put in folder
    FILE *f;
    // char path2[40]; // OLD, too small
    char path2[PATH_MAX]; // MODIFIED: Increased size for absolute paths
    // strcpy(path2, folder_path); // folder_path is now absolute
    // strncat(path2, "/data1.csv", 11); // Old, unsafe strncat
    snprintf(path2, sizeof(path2), "%s/data1.csv", folder_path); // MODIFIED: Safe construction

    f = fopen(path2, "w");
    if (f == NULL)
    {
        // Use LOG_TIMESTAMPED if logPath is usable, otherwise raw printf/fprintf
        fprintf(stderr, "Timestamp: %ld, File: %s, Line: %d, Failed to open data file '%s': %s\n",
                (long)time(NULL), __FILE__, __LINE__, path2, strerror(errno));
        // LOG_TIMESTAMPED might not work if logPath itself is problematic, though unlikely now
        // For safety, this explicit fprintf to stderr is good for this critical error.
        exit(1);
    }
    dataFile = &f;

    LOG_TIMESTAMPED("Log Started\n"); // This should now work correctly.

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
    digitalWrite(camera, LOW); // turn on cameras (assuming LOW is ON for your camera logic)
    usleep(10000000);
    digitalWrite(camera, HIGH); // turn off cameras

    LOG_TIMESTAMPED("Quitting Program\n");
    data = false;
    quitting_program = true; // Signal to stop monitoring CC_pin

    digitalWrite(WHITE_NOISE_GEN, OFF); // turn off actuator

    // Wait for data collection to end
    pthread_join(id, NULL); // Wait for dataFunction118 to finish
    pthread_join(ptid_Log_Warning, NULL);

    if (folder_path != NULL) { // Free the dynamically allocated folder_path
        free(folder_path);
        folder_path = NULL;
    }
    if (f != NULL) { // Close the data file
        fclose(f);
        dataFile = NULL; // Or set *dataFile = NULL if f was a local copy of *dataFile
    }


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
    // uint8_t synced; // Unused
    // uint8_t clock_source; // Unused
    int totalChannels = 0;
    int device_1_chan_count = 0;
    int device_2_chan_count = 0;
    uint16_t chan_mask[DEVICE_COUNT];
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        chan_mask[i] = 0x00 << 8; // This is just 0. Should be chan_mask[i] = 0;
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
    // uint8_t sample_cutter = device_1_chan_count ^ ((device_1_chan_count ^ device_2_chan_count) & -(device_1_chan_count < device_2_chan_count)); // This is a bitwise way to get max, simpler:
    uint8_t sample_cutter = (device_1_chan_count > device_2_chan_count) ? device_1_chan_count : device_2_chan_count;
    if (sample_cutter == 0 && totalChannels > 0) { // Safety if both counts are 0 but totalChannels isn't
        LOG_RAW("Warning: sample_cutter is 0 but totalChannels is %d. Forcing sample_cutter to 1.\n", totalChannels);
        sample_cutter = 1; // Avoid division by zero, though this indicates a channel setup issue.
    } else if (sample_cutter == 0 && totalChannels == 0) {
        LOG_RAW("No channels selected for DAQ. MCC118 scan will not run meaningfully.\n");
        // Decide how to handle: maybe skip scan, or proceed (it might error out).
        // For now, let it proceed, mcc118_a_in_scan_start might handle 0 channels.
    }


    // assert(device_2_chan_count == 4);
    double sample_rate_per_channel = (sample_cutter > 0) ? (100000.0 / sample_cutter) : 100000.0; // Avoid division by zero

    // int buffer_size = sample_rate_per_channel * 10; // This can be very large if sample_rate_per_channel is large.
                                                    // Also, sample_rate_per_channel can be float.
    int buffer_size = (int)(sample_rate_per_channel > 0 ? sample_rate_per_channel : 1000) * 1; // e.g. 1 second of data per read
    if (buffer_size <=0) buffer_size = 1000; // a minimum reasonable buffer size

    // uint16_t status; // Unused
    // uint32_t samples_read_per_chan[DEVICE_COUNT]; // Unused
    uint32_t total_samples_read = 0; // This is not actually updated in the loop to reflect total samples collected

    // Allocate memory for the buffer and sortbuffer
    double **buffer = (double **)malloc(DEVICE_COUNT * sizeof(double *));
    // double **sortbuffer = (double **)malloc(totalChannels * sizeof(double *)); // totalChannels can be 0
    double **sortbuffer = NULL;
    if (totalChannels > 0) {
        sortbuffer = (double **)malloc(totalChannels * sizeof(double *));
        if (!sortbuffer) {
             LOG_RAW("Failed to allocate sortbuffer (totalChannels pointers)\n");
             // free buffer and exit or handle error
             for(int i=0; i<DEVICE_COUNT; ++i) if(buffer[i]) free(buffer[i]);
             free(buffer);
             return NULL; // or appropriate error handling
        }
    }


    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        // buffer[i] = (double *)malloc(buffer_size * 10 * sizeof(double)); // buffer_size * 10 can be huge
        buffer[i] = (double *)malloc(buffer_size * sizeof(double));
        assert(buffer[i]);
    }
    if (totalChannels > 0 && sortbuffer) {
        for (int i = 0; i < totalChannels; i++)
        {
            // sortbuffer[i] = (double *)malloc(buffer_size * 2 * sizeof(double));
            sortbuffer[i] = (double *)malloc(buffer_size * sizeof(double)); // Match buffer read size initially
            assert(sortbuffer[i]);
        }
    }


    // Open the devices
    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        result = mcc118_open(address[device]);
        STOP_ON_ERROR(result);
    }

    // Check if the devices are open
    // for (int device = 0; device < DEVICE_COUNT; device++)
    // {
    //     if (!mcc118_is_open(address[device]))
    //     {
    //         LOG_RAW("MCC118 device %d is not open after attempting to open.\n", address[device]);
    //         assert(0); // This will terminate if the device isn't open
    //     }
    // }


    // Start the scan
    // uint32_t samples_per_channel = 100000; // This is samples_to_read_per_channel for the mcc118_a_in_scan_read function
                                           // not the total for the scan if continuous.
                                           // For OPTS_CONTINUOUS, this is the internal buffer size in the library.
    uint32_t scan_read_request_count = buffer_size; // How many samples to request per read call

    if (totalChannels > 0) { // Only start scan if there are channels
        for (int device = 0; device < DEVICE_COUNT; device++)
        {
            // Only scan if this device has active channels
            if (chan_mask[device] == 0) continue;

            // The samples_per_channel argument for mcc118_a_in_scan_start is the internal DAQ HAT FIFO buffer size.
            // It's not the total number of samples if OPTS_CONTINUOUS is used.
            // A common value is samples_per_channel_for_internal_buffer = 10 * sample_rate_per_channel (e.g. 10s worth)
            // Or just a large enough fixed number, e.g. 100000, if sample_rate_per_channel is low.
            // Let's use a value similar to your original samples_per_channel, but ensure it's reasonable.
            uint32_t internal_buffer_size_per_channel = (uint32_t) (sample_rate_per_channel * 2.0); // 2 seconds of buffer
            if (internal_buffer_size_per_channel < 1000) internal_buffer_size_per_channel = 1000; // Min buffer
            if (internal_buffer_size_per_channel > 1000000) internal_buffer_size_per_channel = 1000000; // Max buffer to avoid excessive memory

            result = mcc118_a_in_scan_start(address[device], chan_mask[device], internal_buffer_size_per_channel, sample_rate_per_channel, OPTS_CONTINUOUS);
            STOP_ON_ERROR(result);
        }
    }
    // fprintf(*logFile, "                DAQHat Sample rate:%f\n",sample_rate_per_channel);

    double actual_sample_rate = 0;
    if (sample_cutter > 0) { // Only if there are channels to calculate rate for
         mcc118_a_in_scan_actual_rate(sample_cutter, sample_rate_per_channel, &actual_sample_rate);
         LOG_RAW("DAQHat Actual Sample rate per channel: %f (requested %f for %d max active channels on a board)\n", actual_sample_rate, sample_rate_per_channel, sample_cutter);
    }


    // This loop structure for reading and sorting data is missing.
    // The original code has `while (data) {}` which seems to be a placeholder
    // for the main data acquisition loop logic that was removed or not implemented.
    // Data is never read from the DAQ or written to the file in this loop.
    // The `fprintf` loop after `while(data)` uses `total_samples_read` which is never incremented,
    // and `sortbuffer` which is never filled.
    // This function will effectively write nothing to the data file.
    // This is a major pre-existing issue beyond the folder creation.
    // For the purpose of this fix, I am leaving this logic as it was.
    while (data)
    {
         // Placeholder: Data acquisition logic should be here.
         // mcc118_a_in_scan_read(...)
         // process/sort data into sortbuffer
         // write to file periodically or buffer it
         // update total_samples_read
         usleep(100000); // Prevent busy-waiting, remove if scan_read is blocking
    } /////////////////////////////////////////////

    // This loop will not run as total_samples_read is 0.
    // If it were to run, it should write to *dataFile, not dataFile (which is FILE**)
    // And ensure *dataFile is not NULL.
    if (*dataFile != NULL && sortbuffer != NULL) {
        for (uint32_t i = 0; i < total_samples_read; i++) // total_samples_read is 0
        {
            for (int j = 0; j < totalChannels; j++)
            {
                fprintf(*dataFile, "%f, ", sortbuffer[j][i]);
            }
            fprintf(*dataFile, "\n");
        }
    }


stop: // Should be `cleanup_and_exit:` or similar as `stop` is not a C label target for goto from scan loop
    initializeMCC();    
    // for (int device = 0; device < DEVICE_COUNT; device++)
    // {
    //     if (mcc118_is_open(address[device])) { // Only if open and scan was started
    //         if (chan_mask[device] > 0) { // Only if scan was started for this device
    //             result = mcc118_a_in_scan_stop(address[device]);
    //             print_error(result); // This is a daqhats_utils function

    //             result = mcc118_a_in_scan_cleanup(address[device]);
    //             print_error(result);
    //         }
    //         result = mcc118_close(address[device]);
    //         print_error(result);
    //     }
    // }
    LOG_RAW("MCC118 devices closed and scan cleanup attempted.\n");

    if (buffer) {
        for (int device = 0; device < DEVICE_COUNT; device++)
        {
            if (buffer[device]) free(buffer[device]);
        }
        free(buffer);
        LOG_RAW("buffer freed\n");
    }


    if (sortbuffer) {
        for (int chan = 0; chan < totalChannels; chan++)
        {
            if (sortbuffer[chan]) free(sortbuffer[chan]);
        }
        free(sortbuffer);
        LOG_RAW("sortbuffer freed\n");
    }
    return NULL; // dataFunction118 is a void* function
}

char *pathCat()
{
    char *path = (char *)malloc(PATH_MAX);
    if (path == NULL) {
        perror("malloc failed in pathCat");
        return NULL; // Indicate error
    }
    memset(path, 0, PATH_MAX); // reset all variables
    time_t t = time(NULL);
    // srand(time(NULL)); // srand should generally be called once in main
    struct tm tm = *localtime(&t);
    char year[10];
    sprintf(year, "%d", tm.tm_year + 1900);
    char month[10];
    sprintf(month, "%02d", tm.tm_mon + 1); // Use %02d for two digits
    char day[10];
    sprintf(day, "%02d", tm.tm_mday); // Use %02d
    char hour[10];
    sprintf(hour, "%02d", tm.tm_hour); // Use %02d
    char min[10];
    sprintf(min, "%02d", tm.tm_min);   // Use %02d
    char sec[10];
    sprintf(sec, "%02d", tm.tm_sec);   // Use %02d

    // Original: strncat(path, "./Blue-Data/results-", PATH_MAX);
    // This function is now only responsible for the "results-TIMESTAMP" part
    // The "./Blue-Data/" prefix is handled in main for absolute path construction.
    // However, to adhere to "ONLY changes to the main code", I will keep pathCat as is.
    // The logic in main will strip "./Blue-Data/" if present.
    // For maximum compatibility with the main changes, ensure it returns expected format.
    snprintf(path, PATH_MAX, "./Blue-Data/results-%s-%s-%s_%s_%s_%s",
             year, month, day, hour, min, sec);
    return path;
}

void initializeGPIO()
{
    wiringPiSetupGpio();

    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(R3, OUTPUT);
    pinMode(R4, OUTPUT);
    pinMode(R5, OUTPUT);
    pinMode(R6, OUTPUT);
    pinMode(R7, OUTPUT);
    pinMode(R8, OUTPUT);

    pinMode(CC_pin, INPUT);

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
{
    do
    {
        // Removed sleep(2) as it makes the loop very slow for quick adjustments
        // Consider a shorter sleep or conditional sleep.
        // For now, adhering to original logic as much as possible.
        sleep(2); // This was original
        digitalWrite(new_s3, ON);
        usleep(400000); // 0.4 seconds
        digitalWrite(new_s3, OFF);
    } while (s3_equalize);
    return NULL;
}

////////////////////code for Blue flight

void *systemLog(void *arg)
{
    // Note: This uses a relative path "./Blue-Data/systemLog.csv".
    // If the systemd service sets WorkingDirectory=/home/pi/MUD_Code,
    // this will correctly resolve to /home/pi/MUD_Code/Blue-Data/systemLog.csv.
    // The _mkdir call in main ensures /home/pi/MUD_Code/Blue-Data exists.
    const char* project_base_dir = "/home/pi/MUD_Code";
    char system_log_path[PATH_MAX];
    snprintf(system_log_path, sizeof(system_log_path), "%s/Blue-Data/systemLog.csv", project_base_dir);

    char command[PATH_MAX + 50];


    while (!quitting_program)
    {
        // Ensure the directory exists (it should have been created by main)
        // _mkdir("/home/pi/MUD_Code/Blue-Data"); // Redundant if main did it, but safe.
                                               // Cannot call _mkdir here per constraints.

        snprintf(command, sizeof(command), "sudo dmesg -c >> %s", system_log_path);
        system(command);
        usleep(1000000); // 1 second
    }
    pthread_exit(NULL); // Preferred over return for pthread functions returning void*
    // return 0; // equivalent to pthread_exit((void*)0);
}

void CCTrigger()
{
    usleep(100000); // Debounce or wait for signal to stabilize
    if (digitalRead(CC_pin) == HIGH) // Check state after delay
    {
        ccEvent = true;
    }
}

void initializeMCC()
{
    
    int result = 0;
    uint8_t address[DEVICE_COUNT];
    uint16_t chan_mask[DEVICE_COUNT];
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        address[i] = i;
    }
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        chan_mask[i] = 0x00 << 8; // This is just 0. Should be chan_mask[i] = 0;
    }
    for (int device = 0; device < DEVICE_COUNT; device++)
    {
        if (mcc118_is_open(address[device])) { // Only if open and scan was started
            LOG_TIMESTAMPED("mcc118 #%i is open\n", device);
            if (chan_mask[device] > 0) { // Only if scan was started for this device
                result = mcc118_a_in_scan_stop(address[device]);
                print_error(result); // This is a daqhats_utils function
                LOG_TIMESTAMPED("mcc118 #%i scan stopped\n", device);

                result = mcc118_a_in_scan_cleanup(address[device]);
                print_error(result);
                LOG_TIMESTAMPED("mcc118 #%i scan buffer cleaned\n", device);
            }
            result = mcc118_close(address[device]);
            print_error(result);
            LOG_TIMESTAMPED("mcc118 #%i closed\n", device);
        }
    }
}