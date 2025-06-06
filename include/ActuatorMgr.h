#ifndef ACTUATOR_MGR_H
#define ACTUATOR_MGR_H

#include "Actuators_classes.h"

/**
 * @brief Manages all actuators in the system, providing high-level control methods.
 */
class ActuatorManager {
public:
    ActuatorManager(Actuator* irrigator, Actuator* pump, Actuator* lamp);

    void applyState();
    void setIrrigatorState(bool state);
    void setPumpState(bool state);
    void setLampState(bool state);

    Actuator* getIrrigator() const;
    Actuator* getPump() const;
    Actuator* getLamp() const;

    

private:
    Actuator* irrigator;    /**< Pointer to the irrigator actuator. */
    Actuator* pump;         /**< Pointer to the pump actuator. */
    Actuator* lamp;         /**< Pointer to the lamp actuator. */
};

#endif // ACTUATOR_MGR_H