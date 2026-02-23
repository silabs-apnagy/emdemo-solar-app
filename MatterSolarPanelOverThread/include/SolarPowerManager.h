/*
 * SolarPowerManager.h
 *
 *  Created on: Oct 21, 2025
 *      Author: arjaro
 */

#pragma once

#include "DataModelTypes.h"
#include <lib/core/CHIPError.h>  

namespace SolarPowerManager{
    CHIP_ERROR Init(chip::EndpointId endpoint);
    void Shutdown(void);

    void UpdatePowerReading(int voltage_V, int current_mA, int power_mW);
    void SendPeriodicEnergyReading(uint64_t periodic_mWh);
    void SendCumulativeEnergyReading(uint64_t cumulative_mWh);
}
