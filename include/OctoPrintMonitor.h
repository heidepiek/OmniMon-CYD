#ifndef _octoPrintMonitor_h
#define _octoPrintMonitor_h

#include <AsyncTCP.h>       // was <ESPAsyncTCP.h> - that's the ESP8266-only variant
#include <ESPAsyncWebServer.h>

// Comment out this line (add // in front) to remove all [TIMING] Serial output.
//#define TIMING_DEBUG

// Both real OctoPrint (Marlin) and Moonraker's own native API are supported now,
// selected per printer via the isMoonraker flag passed to setCurrentPrinter() -
// earlier this file hardcoded the Moonraker endpoint for every printer, which broke
// job info (progress/filename/filament) on printers that are genuine OctoPrint/Marlin.
#define OCTOPRINT_JOB       "/api/job"
#define MOONRAKER_JOB       "/printer/objects/query?print_stats&virtual_sdcard"
#define OCTOPRINT_PRINTER   "/api/printer?exclude=sd"

#define PRINT_STATE_CANCELLING          1
#define PRINT_STATE_CLOSED_OR_ERROR     1 << 1
#define PRINT_STATE_ERROR               1 << 2
#define PRINT_STATE_FINISHING           1 << 3
#define PRINT_STATE_OPERATIONAL         1 << 4
#define PRINT_STATE_PAUSED              1 << 5
#define PRINT_STATE_PAUSING             1 << 6
#define PRINT_STATE_PRINTING            1 << 7
#define PRINT_STATE_READY               1 << 8
#define PRINT_STATE_RESUMING            1 << 9
#define PRINT_STATE_SD_READY            1 << 10

typedef struct OctoPrintMonitorData
{
    // job
    String fileName;
    unsigned int estimatedPrintTime;
    unsigned int filamentLength;
    String jobState;
    float percentComplete;
    unsigned int printTimeElapsed;
    unsigned int printTimeRemaining;
    bool jobLoaded;

    // printer
    float tool0Temp;
    float tool0Target;
    float bedTemp;
    float bedTarget;
    String printState;
    uint16_t printerFlags;

    bool validJobData;
    bool validPrintData;
} PrinterMonitorData;

class OctoPrintMonitor
{
    public:
        //void init(String server, int port, String apiKey, String userName, String password);
        void setCurrentPrinter(String server, int port, String apiKey, String userName, String password, bool isMoonraker);
        void update();
        OctoPrintMonitorData* getCurrentData() { return &data; }

    private:
        void updateJobStatus();
        void updatePrinterStatus();
        int performAPIGet(String apiCall, String& payload);
        void deserialiseJobOctoPrint(String payload);
        void deserialiseJobMoonraker(String payload);
        void deserialisePrint(String payload);

        String apiKey;
        String server;
        String userName;
        String password;
        int port;
        bool isMoonraker;

        OctoPrintMonitorData data;
};

#endif // _octoPrintMonitor_h