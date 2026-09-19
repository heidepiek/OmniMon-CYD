#include <WiFiClient.h>
#include <WiFi.h>          // was <ESP8266WiFi.h>
#include <HTTPClient.h>    // was <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include "OctoPrintMonitor.h"

const int JOB_DECODE_SIZE   = 1536;   // TODO (Moonraker response, includes result/status wrapper)
const int PRINT_DECODE_SIZE = 2048;   // TODO

void OctoPrintMonitor::setCurrentPrinter(String server, int port, String apiKey, String userName, String password, bool isMoonraker)
{
    this->apiKey = apiKey;
    this->server = server;
    this->userName = userName;
    this->password = password;
    this->port = port;
    this->isMoonraker = isMoonraker;
}

void OctoPrintMonitor::update()
{
    updateJobStatus();
    updatePrinterStatus();
}

void OctoPrintMonitor::updateJobStatus()
{
    String result;
    int httpCode;
#ifdef TIMING_DEBUG
    unsigned long startTime = millis();
#endif

    httpCode = performAPIGet(isMoonraker ? MOONRAKER_JOB : OCTOPRINT_JOB, result);

#ifdef TIMING_DEBUG
    Serial.print("[TIMING] updateJobStatus (");
    Serial.print(isMoonraker ? "Moonraker" : "OctoPrint");
    Serial.print(") took ");
    Serial.print(millis() - startTime);
    Serial.println("ms");
#endif
    
    if(httpCode == 200)
    {
        data.validJobData = true;
        if(isMoonraker)
        {
            deserialiseJobMoonraker(result);
        }
        else
        {
            deserialiseJobOctoPrint(result);
        }
    }
    else
    {
        data.validJobData = false;
    }
}

void OctoPrintMonitor::updatePrinterStatus()
{
    String result;
    int httpCode;
#ifdef TIMING_DEBUG
    unsigned long startTime = millis();
#endif

    httpCode = performAPIGet(OCTOPRINT_PRINTER, result);

#ifdef TIMING_DEBUG
    Serial.print("[TIMING] updatePrinterStatus took ");
    Serial.print(millis() - startTime);
    Serial.println("ms");
#endif
    
     if(httpCode == 200)
    {
        data.validPrintData = true;
        deserialisePrint(result);
    }
    else
    {
        data.validPrintData = false;
    }
}

int OctoPrintMonitor::performAPIGet(String apiCall, String& payload)
{
    // must be in this order
    WiFiClient client;
    HTTPClient http;

    http.begin(client, this->server, this->port, apiCall);
    http.setConnectTimeout(1500);  // caps the TCP connect phase - setTimeout() below doesn't cover this on ESP32
    http.setTimeout(2000);         // caps waiting for a response once connected
    http.addHeader("X-Api-Key", this->apiKey);

    if(this->userName != "")
    {
        http.setAuthorization(this->userName.c_str(), this->password.c_str());
    }

    int httpCode = http.GET();

    //Serial.print("HTTP CODE: ");
    //Serial.println(httpCode);

    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {            
            payload = http.getString();
        }        
    }
    
    return httpCode;
}

void OctoPrintMonitor::deserialiseJobOctoPrint(String payload)
{
    // Genuine OctoPrint (Marlin, or any other firmware OctoPrint itself drives)
    // response shape: { "job": { "file": {...}, ... }, "progress": {...}, "state": ... }
    DynamicJsonDocument doc(JOB_DECODE_SIZE);
    deserializeJson(doc, payload);

    data.jobState = (const char*)doc["state"];

    const char* display = doc["job"]["file"]["display"];

    if(display != nullptr)
    {
        data.jobLoaded = true;
        data.estimatedPrintTime = doc["job"]["estimatedPrintTime"];
        data.filamentLength = doc["job"]["filament"]["tool0"]["length"];
        data.fileName = String(display);
        
        data.percentComplete = doc["progress"]["completion"];
        data.printTimeElapsed = doc["progress"]["printTime"];
        data.printTimeRemaining = doc["progress"]["printTimeLeft"];
    }
    else
    {
        data.jobLoaded = false;
    }
}

void OctoPrintMonitor::deserialiseJobMoonraker(String payload)
{
    // Moonraker's native response looks like:
    // { "result": { "status": { "print_stats": {...}, "virtual_sdcard": {...} } } }
    DynamicJsonDocument doc(JOB_DECODE_SIZE);
    deserializeJson(doc, payload);

    JsonObject status = doc["result"]["status"];
    const char* filename = status["print_stats"]["filename"];
    const char* state = status["print_stats"]["state"];

    data.jobState = (state != nullptr) ? String(state) : String("standby");

    if(filename != nullptr && strlen(filename) > 0)
    {
        data.jobLoaded = true;
        data.fileName = String(filename);

        float progress = status["virtual_sdcard"]["progress"] | 0.0f;       // 0.0 - 1.0
        float elapsed  = status["print_stats"]["total_duration"] | 0.0f;    // seconds since start
        float filamentUsed = status["print_stats"]["filament_used"] | 0.0f; // mm used so far

        data.percentComplete  = progress * 100.0f;
        data.printTimeElapsed = (unsigned int)elapsed;
        data.filamentLength   = (unsigned int)filamentUsed;

        // Moonraker doesn't report an OctoPrint-style estimated/remaining time directly,
        // so it's derived here from current progress vs elapsed time (linear estimate,
        // same approach community tools like moonraker-octoprint-enhanced use).
        if(progress > 0.0f)
        {
            float estimatedTotal = elapsed / progress;
            data.estimatedPrintTime = (unsigned int)estimatedTotal;
            data.printTimeRemaining = (unsigned int)max(0.0f, estimatedTotal - elapsed);
        }
        else
        {
            data.estimatedPrintTime = 0;
            data.printTimeRemaining = 0;
        }
    }
    else
    {
        data.jobLoaded = false;
    }
}

void OctoPrintMonitor::deserialisePrint(String payload)
{
    DynamicJsonDocument doc(PRINT_DECODE_SIZE);
    deserializeJson(doc, payload);

    data.tool0Temp = doc["temperature"]["tool0"]["actual"];
    data.tool0Target= doc["temperature"]["tool0"]["target"];

    data.bedTemp = doc["temperature"]["bed"]["actual"];
    data.bedTarget = doc["temperature"]["bed"]["target"];

    data.printState = (const char*)doc["state"]["text"];
    data.printerFlags = 0;
    
    if(doc["state"]["flags"]["cancelling"])
    {
        data.printerFlags |= PRINT_STATE_CANCELLING;
    }
    if(doc["state"]["flags"]["closedOrError"])
    {
        data.printerFlags |= PRINT_STATE_CLOSED_OR_ERROR;
    }
    if(doc["state"]["flags"]["error"])
    {
        data.printerFlags |= PRINT_STATE_ERROR;
    }
    if(doc["state"]["flags"]["finishing"])
    {
        data.printerFlags |= PRINT_STATE_FINISHING;
    }
    if(doc["state"]["flags"]["operational"])
    {
        data.printerFlags |= PRINT_STATE_OPERATIONAL;
    }
    if(doc["state"]["flags"]["paused"])
    {
        data.printerFlags |= PRINT_STATE_PAUSED;
    }
    if(doc["state"]["flags"]["pausing"])
    {
        data.printerFlags |= PRINT_STATE_PAUSING;
    }
    if(doc["state"]["flags"]["printing"])
    {
        data.printerFlags |= PRINT_STATE_PRINTING;
    }
    if(doc["state"]["flags"]["ready"])
    {
        data.printerFlags |= PRINT_STATE_READY;
    }
    if(doc["state"]["flags"]["resuming"])
    {
        data.printerFlags |= PRINT_STATE_RESUMING;
    }
    if(doc["state"]["flags"]["sdReady"])
    {
        data.printerFlags |= PRINT_STATE_SD_READY;
    }
}

