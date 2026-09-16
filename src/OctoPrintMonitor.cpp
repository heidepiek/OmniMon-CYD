#include <WiFiClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include "OctoPrintMonitor.h"

const int JOB_DECODE_SIZE   = 1536;   // TODO (Moonraker response, includes result/status wrapper)
const int PRINT_DECODE_SIZE = 2048;   // TODO

void OctoPrintMonitor::setCurrentPrinter(String server, int port, String apiKey, String userName, String password)
{
    this->apiKey = apiKey;
    this->server = server;
    this->userName = userName;
    this->password = password;
    this->port = port;
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
    
    httpCode = performAPIGet(OCTOPRINT_JOB, result);
    
    if(httpCode == 200)
    {
        data.validJobData = true;
        deserialiseJob(result);
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

    httpCode = performAPIGet(OCTOPRINT_PRINTER, result);
    
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
    http.setTimeout(2000);
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

void OctoPrintMonitor::deserialiseJob(String payload)
{
    if (payload.length() == 0) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        return; // Voorkom crash bij corrupte data
    }

    JsonObject status = doc["result"]["status"];
    if (status.isNull()) return;

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
    if (payload.length() == 0) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        return; // Voorkom crash bij corrupte data
    }

    if (doc["temperature"].isNull() || doc["state"].isNull()) return;

    data.tool0Temp = doc["temperature"]["tool0"]["actual"] | 0.0f;
    data.tool0Target = doc["temperature"]["tool0"]["target"] | 0.0f;

    data.bedTemp = doc["temperature"]["bed"]["actual"] | 0.0f;
    data.bedTarget = doc["temperature"]["bed"]["target"] | 0.0f;

    const char* stateText = doc["state"]["text"];
    data.printState = (stateText != nullptr) ? String(stateText) : String("Unknown");
    data.printerFlags = 0;
    
    JsonObject flags = doc["state"]["flags"];
    if(!flags.isNull())
    {
        if(flags["cancelling"]) data.printerFlags |= PRINT_STATE_CANCELLING;
        if(flags["closedOrError"]) data.printerFlags |= PRINT_STATE_CLOSED_OR_ERROR;
        if(flags["error"]) data.printerFlags |= PRINT_STATE_ERROR;
        if(flags["finishing"]) data.printerFlags |= PRINT_STATE_FINISHING;
        if(flags["operational"]) data.printerFlags |= PRINT_STATE_OPERATIONAL;
        if(flags["paused"]) data.printerFlags |= PRINT_STATE_PAUSED;
        if(flags["pausing"]) data.printerFlags |= PRINT_STATE_PAUSING;
        if(flags["printing"]) data.printerFlags |= PRINT_STATE_PRINTING;
        if(flags["ready"]) data.printerFlags |= PRINT_STATE_READY;
        if(flags["resuming"]) data.printerFlags |= PRINT_STATE_RESUMING;
        if(flags["sdReady"]) data.printerFlags |= PRINT_STATE_SD_READY;
    }
}