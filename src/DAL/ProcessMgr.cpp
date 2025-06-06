#include "ProcessMgr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Arduino.h>

#define SENSOR_PIR_COOL_DOWN_TIME (5000) 
#define SENSOR_WATER_WELL_FULL  (false) /* Assuming true means well is full */
#define SENSOR_WATER_WELL_EMPTY (true) /* Assuming true means well is empty */

/**
 * @brief Handles the activation and deactivation of the lamp based on PIR sensor and light sensor states.
 * @param data Pointer to the SystemData structure containing sensor and actuator objects.
 */
void LampActivationCtrl(SystemData* data) {
    static uint32_t lastPirTriggerTime = 0;
    static bool presenceDetected = false;
    static bool pirWentLow = false;

    uint32_t currentMillis = millis();

    if (xSemaphoreTake(xSystemDataMutex, portMAX_DELAY)) {
        bool lightState = data->sensorMgr->getLightSensorValue();
        bool pirState = data->sensorMgr->getPirSensorValue();
        bool lampState = data->actuatorMgr->getLamp()->getOutstate();

        if (pirState) {
            /* If PIR detects presence, activate Lamp immediately */
            presenceDetected = true;
            pirWentLow = false; /** Reset cooldown tracking */
            data->actuatorMgr->setLampState(true);
            lastPirTriggerTime = currentMillis; /** Reset PIR cooldown timer */
        } else if (lightState && !lampState) {
            /* If it's dark AND Lamp is OFF, activate Lamp */
            data->actuatorMgr->setLampState(true);
        } else if (!lightState && !presenceDetected) { 
            /* If light sensor detects LIGHT and PIR is NOT detecting presence, turn Lamp OFF immediately */
            presenceDetected = false;
            pirWentLow = false;
            data->actuatorMgr->setLampState(false);
        } else if (!pirState && presenceDetected && !pirWentLow) {
            /* If PIR stopped detecting presence, start cooldown */
            pirWentLow = true;
            lastPirTriggerTime = currentMillis;
        } else if (pirWentLow && (currentMillis - lastPirTriggerTime >= SENSOR_PIR_COOL_DOWN_TIME)) { 
            /* If PIR has been LOW for cooldown time, only turn Lamp OFF if light sensor does NOT require it to stay ON */
            presenceDetected = false;
            pirWentLow = false;
            if (!lightState) {
                data->actuatorMgr->setLampState(false);
            }
        }

        data->PirPresenceDetected = presenceDetected;
        xSemaphoreGive(xSystemDataMutex);
    }
}

/**
 * @brief Handles the activation and deactivation of the pump based on water level sensor readings.
 * @param data Pointer to the SystemData structure containing sensor and actuator objects.
 */
void PumpActivationCtrl(SystemData* data) {
    static bool pumpState = false;
    bool wellSensorState = data->sensorMgr->getWellSensorValue();

    if (xSemaphoreTake(xSystemDataMutex, portMAX_DELAY)) {
        uint16_t levelValue = data->sensorMgr->getLevelSensorValue();
        uint16_t levelPercentage = ((levelValue - (SENSOR_LVL_ADC_0_V + SENSOR_LVL_THRESHOLD_V)) * 100) /
                                    ((SENSOR_LVL_ADC_100_V - SENSOR_LVL_THRESHOLD_V) - (SENSOR_LVL_ADC_0_V + SENSOR_LVL_THRESHOLD_V));

        if (levelValue >= SENSOR_LVL_ADC_100_V) {
            levelPercentage = 100; 
        } else if (levelValue <= SENSOR_LVL_ADC_0_V) {
            levelPercentage = 0; 
        } else {
            if ( (levelPercentage <= data->minLevelPercentage) && (wellSensorState == SENSOR_WATER_WELL_FULL) ) {
                /* If level is below minimum and well is Full, turn pump ON */
                pumpState = true;
            } else if ( (levelPercentage >= data->maxLevelPercentage) || (wellSensorState == SENSOR_WATER_WELL_EMPTY) ) {
                /* If level is above maximum or well is Empty, turn pump OFF */
                pumpState = false;
            } else {
                /* If level is between min and max, keep pump state unchanged */
                pumpState = data->actuatorMgr->getPump()->getOutstate();
            }
        }

        if (levelPercentage >= 100) {
            levelPercentage = 100;
        }

        data->levelPercentage = levelPercentage;
        data->actuatorMgr->setPumpState(pumpState);
        xSemaphoreGive(xSystemDataMutex);
    }
}

/**
 * @brief Handles the activation and deactivation of the irrigator based on temperature and humidity sensor readings.
 * @param data Pointer to the SystemData structure containing sensor and actuator objects.
 */
void IrrigatorActivationCtrl(SystemData* data) {
    static bool irrigatorState = false;

    if (xSemaphoreTake(xSystemDataMutex, portMAX_DELAY)) {
        double temperature = data->sensorMgr->getTemperature();
        double humidity = data->sensorMgr->getHumidity();

        if ( (temperature >= data->hotTemperature) && (humidity <= data->lowHumidity) && (data->levelPercentage >= data->minLevelPercentage) ) {
            if (!irrigatorState) {
                data->actuatorMgr->setIrrigatorState(true);
                irrigatorState = true;
            }
        } else if (temperature < data->hotTemperature - 2 || humidity > data->lowHumidity + 5) {
            if (irrigatorState) {
                data->actuatorMgr->setIrrigatorState(false);
                irrigatorState = false;
            }
        } else {
            /* If conditions are not met, keep the irrigator OFF */
            data->actuatorMgr->setIrrigatorState(false);
            irrigatorState = false;
        }

        xSemaphoreGive(xSystemDataMutex);
    }
}

/**
 * @brief processes button inputs for system control and settings.
 * @param data Pointer to the SystemData structure containing sensor and actuator objects.
 */
void pButtonsCtrl(SystemData* data) {
    static uint32_t lastButtonPressTime = 0;
    static uint8_t currentSettingMenu = 0;
    uint8_t* Levelsettings[] = {
        &data->maxLevelPercentage,
        &data->minLevelPercentage,
    };
    uint8_t* TempHumsettings[] = {
        &data->hotTemperature,
        &data->lowHumidity
    };

    /* Protect shared variable access */
    if (xSemaphoreTake(xSystemDataMutex, portMAX_DELAY)) {
        uint32_t currentMillis = millis();
        bool SelectbuttonState = !(data->sensorMgr->getButtonSelectorValue()); /* (pressed = LOW, released = HIGH) */
        bool escButtonState = !(data->sensorMgr->getButtonEscValue()); /* (pressed = LOW, released = HIGH) */
        bool upButtonState = !(data->sensorMgr->getButtonUpValue()); /* (pressed = LOW, released = HIGH) */  
        bool downButtonState = !(data->sensorMgr->getButtonDownValue()); /* (pressed = LOW, released = HIGH) */

        if (SelectbuttonState && (currentMillis - lastButtonPressTime > 300)) {
            lastButtonPressTime = currentMillis;
            if (data->currentDisplayDataSelec == SCREEN_LVL_SETT_MENU) {
                /* Navigate through systems settings screen menu */ 
                data->currentSettingMenu++;
                if (data->currentSettingMenu >= sizeof(Levelsettings) / sizeof(Levelsettings[0])) {
                    /* Reset to the first setting */
                    data->currentSettingMenu = 0;
                }
            } else if(data->currentDisplayDataSelec == SCREEN_TEMP_HUM_SETT_MENU) {
                /* Navigate through temperature and humidity settings screen menu */
                data->currentSettingMenu++;
                if (data->currentSettingMenu >= sizeof(TempHumsettings) / sizeof(TempHumsettings[0])) {
                    /* Reset to the first setting */
                    data->currentSettingMenu = 0;
                }
            } else {
                /* Change display main screens by select button if wifi setting menus are not being used */
                if( (data->currentDisplayDataSelec != SCREEN_WIFI_SETT_MENU) && (data->currentDisplayDataSelec != SCREEN_WIFI_SETT_SUB_MENU) ) {
                    data->currentDisplayDataSelec = static_cast<pb1Selector>((data->currentDisplayDataSelec + 1) % SCREEN_SELECT_NEXT);
                }
            }
        }

        if (escButtonState && (currentMillis - lastButtonPressTime > 300)) {
            lastButtonPressTime = currentMillis;
            data->currentSettingMenu = 0;
            if(data->currentDisplayDataSelec == SCREEN_LVL_PUMP_DATA) {
                /* If the current screen is the level and pump data screen, go to the temperature and humidity settings */
                data->currentDisplayDataSelec = SCREEN_LVL_SETT_MENU;
            } else if (data->currentDisplayDataSelec == SCREEN_TEMP_HUM_IRR_DATA) {
                /* If the current screen is the temperature and humidity data screen, go to the level settings */
                data->currentDisplayDataSelec = SCREEN_TEMP_HUM_SETT_MENU;
            } else if(data->currentDisplayDataSelec == SCREEN_WIFI_STATUS) {
                /* If the current screen is the WiFi status screen, go to the wifi settings */
                data->currentDisplayDataSelec = SCREEN_WIFI_SETT_MENU;
            } else if (data->currentDisplayDataSelec == SCREEN_LVL_SETT_MENU) {
                /* If the current screen is the settings screen, go back to the previous screen */
                data->currentDisplayDataSelec = SCREEN_LVL_PUMP_DATA;
            } else if(data->currentDisplayDataSelec == SCREEN_TEMP_HUM_SETT_MENU) {
                /* If the current screen is the settings screen, go back to the previous screen */
                data->currentDisplayDataSelec = SCREEN_TEMP_HUM_IRR_DATA;
            } else if(data->currentDisplayDataSelec == SCREEN_WIFI_SETT_MENU) {
                /* If the current screen is the settings screen, go back to the previous screen */
                data->currentDisplayDataSelec = SCREEN_WIFI_STATUS;
            } else {
                /* Do nothing */
            }
        }

        if (data->currentDisplayDataSelec == SCREEN_LVL_SETT_MENU) {
            if (upButtonState && (currentMillis - lastButtonPressTime > 300)) {
                lastButtonPressTime = currentMillis;
                /* Increment the current setting value */
                if (*Levelsettings[data->currentSettingMenu] < 100) {
                    (*Levelsettings[data->currentSettingMenu])++;
                }
            }

            if (downButtonState && (currentMillis - lastButtonPressTime > 300)) {
                lastButtonPressTime = currentMillis;
                /* Decrement the current setting value */ 
                if (*Levelsettings[data->currentSettingMenu] > 0) {
                    (*Levelsettings[data->currentSettingMenu])--;
                }
            }
        } else if (data->currentDisplayDataSelec == SCREEN_TEMP_HUM_SETT_MENU) {
            if (upButtonState && (currentMillis - lastButtonPressTime > 300)) {
                lastButtonPressTime = currentMillis;
                /* Increment the current setting value */
                if (*TempHumsettings[data->currentSettingMenu] < 100) {
                    (*TempHumsettings[data->currentSettingMenu])++;
                }
            }

            if (downButtonState && (currentMillis - lastButtonPressTime > 300)) {
                lastButtonPressTime = currentMillis;
                /* Decrement the current setting value */ 
                if (*TempHumsettings[data->currentSettingMenu] > 0) {
                    (*TempHumsettings[data->currentSettingMenu])--;
                }
            }
        } else {
            /* Do nothing */
        }

        xSemaphoreGive(xSystemDataMutex);
    }
}