#include "SensorMgr.h"

/**
 * @brief Constructs the SensorManager object and initializes all sensors.
 * @param levelSensor Pointer to the AnalogSensor object for the level sensor.
 * @param tempHumSensor Pointer to the Dht11TempHumSens object.
 * @param pirSensor Pointer to the DigitalSensor object for the PIR sensor.
 * @param lightSensor Pointer to the DigitalSensor object for the light sensor.
 * @param buttonSelector Pointer to the DigitalSensor object for the button selector.
 * @param buttonEsc Pointer to the DigitalSensor object for the ESC button.
 * @param buttonUp Pointer to the DigitalSensor object for the UP button.
 * @param buttonDown Pointer to the DigitalSensor object for the DOWN button.
 * @param wellSensor Pointer to the DigitalSensor object for the well sensor.
 */
SensorManager::SensorManager(AnalogSensor* levelSensor, 
                             Dht11TempHumSens* tempHumSensor, 
                             DigitalSensor* pirSensor, 
                             DigitalSensor* lightSensor, 
                             DigitalSensor* buttonSelector, 
                             DigitalSensor* buttonEsc, 
                             DigitalSensor* buttonUp, 
                             DigitalSensor* buttonDown,
                             DigitalSensor* wellSensor) 
                             : 
                             levelSensor(levelSensor), 
                             tempHumSensor(tempHumSensor), 
                             pirSensor(pirSensor),
                             lightSensor(lightSensor), 
                             buttonSelector(buttonSelector), 
                             buttonEsc(buttonEsc),
                             buttonUp(buttonUp),
                             buttonDown(buttonDown),
                             wellSensor(wellSensor) {}

/**
 * @brief Reads the level sensor and updates the internal value.
 */
void SensorManager::readLevelSensor() {
    levelValue = levelSensor->readRawValue();
}

/**
 * @brief Reads the temperature and humidity sensors and updates the internal values.
 */
void SensorManager::readDht11TempHumSens() {
    temperature = tempHumSensor->readValueTemperature();
    humidity = tempHumSensor->readValueHumidity();
}

/**
 * @brief Reads the PIR sensor and updates the internal value.
 */
void SensorManager::readPirSensor() {
    pirValue = pirSensor->readRawValue();
}

/**
 * @brief Reads the light sensor and updates the internal value.
 */
void SensorManager::readLightSensor() {
    lightValue = lightSensor->readRawValue();
}

/**
 * @brief Reads the button selector and updates the internal value.
 */
void SensorManager::readButtonSelector() {
    buttonValue = buttonSelector->readRawValue();
}

/**
 * @brief Reads the button ESC and updates the internal value.
 */
void SensorManager::readButtonEsc() {
    buttonEscValue = buttonEsc->readRawValue();
}

/**
 * @brief Reads the button UP and updates the internal value.
 */
void SensorManager::readButtonUp() {
    buttonUpValue = buttonUp->readRawValue();
}

/**
 * @brief Reads the button DOWN and updates the internal value.
 */
void SensorManager::readButtonDown() {
    buttonDownValue = buttonDown->readRawValue();
}

/**
 * @brief Reads the well sensor and updates the internal value.
 * 
 */
void SensorManager::readWellSensor() {
    /* Invert the value since the well sensor is active LOW */
    wellSensorValue = !(wellSensor->readRawValue());  
}

/**
 * @brief Gets the last recorded value of the level sensor.
 * @return The raw ADC value of the level sensor.
 */
uint16_t SensorManager::getLevelSensorValue() const {
    return levelValue;
}

/**
 * @brief Gets the last recorded temperature value.
 * @return The temperature in degrees Celsius.
 */
double SensorManager::getTemperature() const {
    return temperature;
}

/**
 * @brief Gets the last recorded humidity value.
 * @return The humidity percentage.
 */
double SensorManager::getHumidity() const {
    return humidity;
}

/**
 * @brief Gets the last recorded value of the PIR sensor.
 * @return The digital state of the PIR sensor (true for HIGH, false for LOW).
 */
bool SensorManager::getPirSensorValue() const {
    return pirValue;
}

/**
 * @brief Gets the last recorded value of the light sensor.
 * @return The digital state of the light sensor (true for HIGH, false for LOW).
 */
bool SensorManager::getLightSensorValue() const {
    return lightValue;
}

/**
 * @brief Gets the last recorded value of the button selector.
 * @return The digital state of the button selector (true for HIGH, false for LOW).
 */
bool SensorManager::getButtonSelectorValue() const {
    return buttonValue;
}

/**
 * @brief Gets the last recorded value of the button ESC.
 * @return The digital state of the button ESC (true for HIGH, false for LOW).
 */
bool SensorManager::getButtonEscValue() const {
    return buttonEscValue;
}

/**
 * @brief Gets the last recorded value of the button UP.
 * @return The digital state of the button UP (true for HIGH, false for LOW).
 */
bool SensorManager::getButtonUpValue() const {
    return buttonUpValue;
}

/**
 * @brief Gets the last recorded value of the button DOWN.
 * @return The digital state of the button DOWN (true for HIGH, false for LOW).
 */
bool SensorManager::getButtonDownValue() const {
    return buttonDownValue;
}

/**
 * @brief Gets the last recorded value of the well sensor.
 * @return The digital state of the well sensor (true for HIGH, false for LOW).
 */
bool SensorManager::getWellSensorValue() const {
    return wellSensorValue;
}

/**
 * @brief Gets the pointer to the Dht11TempHumSens object.
 * @return Pointer to the Dht11TempHumSens object.
 */
Dht11TempHumSens* SensorManager::getTempHumSensor() const {
    return tempHumSensor;
}