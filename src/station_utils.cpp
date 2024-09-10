#include "station_utils.h"
#include "battery_utils.h"
#include "aprs_is_utils.h"
#include "configuration.h"
#include "lora_utils.h"
#include "utils.h"
#include <vector>

extern Configuration            Config;
extern uint32_t                 lastRxTime;
extern String                   fourthLine;
extern bool                     shouldSleepLowVoltage;

uint32_t lastTxTime             = millis();
std::vector<LastHeardStation>   lastHeardStations;
std::vector<String>             outputPacketBuffer;
std::vector<Packet25SegBuffer>  packet25SegBuffer;


namespace STATION_Utils {

    void deleteNotHeard() {
        std::vector<LastHeardStation>  lastHeardStation_temp;
        for (int i = 0; i < lastHeardStations.size(); i++) {
            if (millis() - lastHeardStations[i].lastHeardTime < Config.rememberStationTime * 60 * 1000) {
                lastHeardStation_temp.push_back(lastHeardStations[i]);
            }
        }
        lastHeardStations.clear();
        for (int j = 0; j < lastHeardStation_temp.size(); j++) {
            lastHeardStations.push_back(lastHeardStation_temp[j]);
        }
        lastHeardStation_temp.clear();
    }

    void updateLastHeard(const String& station) {
        deleteNotHeard();
        bool stationHeard = false;
        for (int i = 0; i < lastHeardStations.size(); i++) {
            if (lastHeardStations[i].station == station) {
                lastHeardStations[i].lastHeardTime = millis();
                stationHeard = true;
            }
        }
        if (!stationHeard) {
            LastHeardStation lastStation;
            lastStation.lastHeardTime   = millis();
            lastStation.station         = station;
            lastHeardStations.push_back(lastStation);
        }

        fourthLine = "Stations (";
        fourthLine += String(Config.rememberStationTime);
        fourthLine += "min) = ";
        if (lastHeardStations.size() < 10) {
            fourthLine += " ";
        }
        fourthLine += String(lastHeardStations.size());
    }

    bool wasHeard(const String& station) {
        deleteNotHeard();
        for (int i = 0; i < lastHeardStations.size(); i++) {
            if (lastHeardStations[i].station == station) {
                Utils::println(" ---> Listened Station");
                return true;
            }
        }
        Utils::println(" ---> Station not Heard for last 30 min (Not Tx)\n");
        return false;
    }

    void clean25SegBuffer() {
        if (!packet25SegBuffer.empty()) {
            if ((millis() - packet25SegBuffer[0].receivedTime) >  25 * 1000) {
                packet25SegBuffer.erase(packet25SegBuffer.begin());
            }
        }
    }

    bool check25SegBuffer(const String& station, const String& textMessage) {
        bool shouldBeIgnored = false;
        if (!packet25SegBuffer.empty()) {
            for (int i = 0; i < packet25SegBuffer.size(); i++) {
                if (packet25SegBuffer[i].station == station && packet25SegBuffer[i].payload == textMessage) {
                    shouldBeIgnored = true;
                }
            }
        }
        if (shouldBeIgnored) {
            return false;
        } else {
            Packet25SegBuffer packet;
            packet.receivedTime = millis();
            packet.station      = station;
            packet.payload      = textMessage;
            packet25SegBuffer.push_back(packet);
            return true;
        }
    }

    void processOutputPacketBuffer() {
        int timeToWait   = 3 * 1000;      // 3 segs between packet Tx and also Rx ???
        uint32_t lastRx  = millis() - lastRxTime;
        uint32_t lastTx  = millis() - lastTxTime;
        if (outputPacketBuffer.size() > 0 && lastTx > timeToWait && lastRx > timeToWait) {
            LoRa_Utils::sendNewPacket(outputPacketBuffer[0]);
            outputPacketBuffer.erase(outputPacketBuffer.begin());
            lastTxTime = millis();
        }
        if (shouldSleepLowVoltage) {
            while (outputPacketBuffer.size() > 0) {
                LoRa_Utils::sendNewPacket(outputPacketBuffer[0]);
                outputPacketBuffer.erase(outputPacketBuffer.begin());
                delay(4000);
            }
        }
    }

    void addToOutputPacketBuffer(const String& packet) {
        outputPacketBuffer.push_back(packet);
    }

}