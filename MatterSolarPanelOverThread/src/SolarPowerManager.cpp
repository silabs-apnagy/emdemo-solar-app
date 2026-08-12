/*
 * SolarPowerManager.cpp
 *
 *  Created on: Oct 21, 2025
 *      Author: arjaro
 */

#include "SolarPowerManager.h"
#include "AppEvent.h"

#include <ElectricalPowerMeasurementDelegate.h>
#include <PowerTopologyDelegate.h>

#include <app/clusters/electrical-energy-measurement-server/CodegenIntegration.h>
#include "power-source-server.h"

#include "Server.h"
#include "Accessors.h"
#include "cluster-enums.h"
#include <app/reporting/reporting.h>

#include <lib/support/CodeUtils.h>
#include <memory>

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ElectricalPowerMeasurement;
using namespace chip::app::Clusters::ElectricalEnergyMeasurement;
using namespace chip::app::Clusters::ElectricalEnergyMeasurement::Structs;
using namespace chip::app::Clusters::PowerTopology;
using namespace chip::app::Clusters::PowerSource;

namespace{
EndpointId solar_power_endpoint = kInvalidEndpointId;

std::unique_ptr<ElectricalPowerMeasurementDelegate> gEPMDelegate;
std::unique_ptr<ElectricalPowerMeasurementInstance> gEPMInstance;
std::unique_ptr<PowerTopologyDelegate> gPTDelegate;
std::unique_ptr<PowerTopologyInstance> gPTInstance;
std::unique_ptr<ElectricalEnergyMeasurementAttrAccess> gEEMAttrAccess;

CHIP_ERROR InitPowerMeasurement(EndpointId endpoint);
CHIP_ERROR InitEnergyMeasurement(EndpointId endpoint);
CHIP_ERROR InitPowerTopology(EndpointId endpoint);
CHIP_ERROR InitPowerSource(EndpointId endpoint);

}

namespace SolarPowerManager{

CHIP_ERROR Init(EndpointId endpoint)
{
	CHIP_ERROR ret;

	solar_power_endpoint = endpoint;

	ret = InitPowerMeasurement(solar_power_endpoint);
	if(ret != CHIP_NO_ERROR) {
		return ret;
	}
	ret = InitEnergyMeasurement(solar_power_endpoint);
	if(ret != CHIP_NO_ERROR) {
		return ret;
	}
	ret = InitPowerTopology(solar_power_endpoint);
	if(ret != CHIP_NO_ERROR) {
		return ret;
	}
	ret = InitPowerSource(solar_power_endpoint);
	if(ret != CHIP_NO_ERROR) {
		return ret;
	}

	return CHIP_NO_ERROR;
}

void Shutdown(void)
{
    ChipLogDetail(AppServer, "Solar Power: Shutdown()");

    if (gEPMInstance)
    {
        gEPMInstance->Shutdown();
        gEPMInstance.reset();
    }
    if (gEPMDelegate)
    {
        gEPMDelegate.reset();
    }
    if (gPTInstance)
    {
        gPTInstance->Shutdown();
        gPTInstance.reset();
    }
    if (gPTDelegate)
    {
        gPTDelegate.reset();
    }
    if (gEEMAttrAccess)
    {
        gEEMAttrAccess->Shutdown();
        gEEMAttrAccess.reset();
    }
}

void UpdatePowerReading(int voltage_V, int current_mA, int power_mW)
{
	gEPMDelegate->SetVoltage(voltage_V);
	gEPMDelegate->SetActiveCurrent(current_mA);
	gEPMDelegate->SetActivePower(power_mW);
}

void SendPeriodicEnergyReading(uint64_t periodic_energy)
{
	EnergyMeasurementStruct::Type energyExported;
	energyExported.startTimestamp.ClearValue();
	energyExported.startSystime.ClearValue();
	energyExported.energy = periodic_energy;
	System::Clock::Milliseconds64 system_time_ms = std::chrono::duration_cast<System::Clock::Milliseconds64>(chip::Server::GetInstance().TimeSinceInit());
	uint64_t nowMS = static_cast<uint64_t>(system_time_ms.count());
	energyExported.endSystime.SetValue(nowMS);
	if(!NotifyPeriodicEnergyMeasured(solar_power_endpoint, NullNullable, MakeNullable(energyExported))){
		//log failure?
	}
	MatterReportingAttributeChangeCallback(solar_power_endpoint, ElectricalEnergyMeasurement::Id, ElectricalEnergyMeasurement::Attributes::PeriodicEnergyExported::Id);
}
void SendCumulativeEnergyReading(uint64_t cumulative_energy)
{
	EnergyMeasurementStruct::Type energyExported;
	energyExported.startTimestamp.ClearValue();
	energyExported.startSystime.ClearValue();
	energyExported.energy = cumulative_energy;
	System::Clock::Milliseconds64 system_time_ms = std::chrono::duration_cast<System::Clock::Milliseconds64>(chip::Server::GetInstance().TimeSinceInit());
	uint64_t nowMS = static_cast<uint64_t>(system_time_ms.count());
	energyExported.endSystime.SetValue(nowMS);
	if(!NotifyCumulativeEnergyMeasured(solar_power_endpoint, NullNullable, MakeNullable(energyExported))){
		//log failure?
	}
	MatterReportingAttributeChangeCallback(solar_power_endpoint, ElectricalEnergyMeasurement::Id, ElectricalEnergyMeasurement::Attributes::CumulativeEnergyExported::Id);
}

}

namespace {
CHIP_ERROR InitPowerMeasurement(EndpointId endpoint)
{
	if (gEPMDelegate || gEPMInstance) {
		ChipLogError(AppServer, "EPM Instance or Delegate already exist.");
		return CHIP_ERROR_INCORRECT_STATE;
	}
	gEPMDelegate = std::make_unique<ElectricalPowerMeasurementDelegate>();
	if (!gEPMDelegate) {
		ChipLogError(AppServer, "Failed to allocate memory for EPM Delegate");
		return CHIP_ERROR_NO_MEMORY;
	}

	/* Manufacturer may optionally not support all features, commands & attributes */
	/* Turning on all optional features and attributes for test certification purposes */
	gEPMInstance = std::make_unique<ElectricalPowerMeasurementInstance>(
			endpoint,
			*gEPMDelegate,
			BitMask<ElectricalPowerMeasurement::Feature, uint32_t>(ElectricalPowerMeasurement::Feature::kAlternatingCurrent),
			BitMask<ElectricalPowerMeasurement::OptionalAttributes, uint32_t>(
					ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeVoltage,
					ElectricalPowerMeasurement::OptionalAttributes::kOptionalAttributeActiveCurrent));

	CHIP_ERROR err = gEPMInstance->Init(); /* Register Attribute & Command handlers */
	if (err != CHIP_NO_ERROR){
		ChipLogError(AppServer, "Init failed on gEPMInstance");
		gEPMInstance.reset();
		gEPMDelegate.reset();
		return err;
	}

	gEPMDelegate->SetVoltage(0);
	gEPMDelegate->SetActiveCurrent(0);
	gEPMDelegate->SetActivePower(0);

	return CHIP_NO_ERROR;
}
CHIP_ERROR InitEnergyMeasurement(EndpointId endpoint)
{
	if (gEEMAttrAccess) {
		ChipLogError(AppServer, "EEM attribute access already exist.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	gEEMAttrAccess = std::make_unique<ElectricalEnergyMeasurementAttrAccess>(
			BitMask<ElectricalEnergyMeasurement::Feature, uint32_t>(
					ElectricalEnergyMeasurement::Feature::kExportedEnergy,
					ElectricalEnergyMeasurement::Feature::kCumulativeEnergy,
					ElectricalEnergyMeasurement::Feature::kPeriodicEnergy),
			BitMask<ElectricalEnergyMeasurement::OptionalAttributes, uint32_t>(
					ElectricalEnergyMeasurement::OptionalAttributes::kOptionalAttributeCumulativeEnergyReset),
			endpoint);

	// Create an accuracy entry which is between +/-0.5 and +/- 5% across the range of all possible energy readings
	ElectricalEnergyMeasurement::Structs::MeasurementAccuracyRangeStruct::Type energyAccuracyRanges[] = {
			{ .rangeMin   = 0,
					.rangeMax   = 1'000'000'000'000'000, // 1 million Mwh
					.percentMax = MakeOptional(static_cast<chip::Percent100ths>(500)),
					.percentMin = MakeOptional(static_cast<chip::Percent100ths>(50)) }
	};

	ElectricalEnergyMeasurement::Structs::MeasurementAccuracyStruct::Type accuracy = {
			.measurementType  = MeasurementTypeEnum::kElectricalEnergy,
			.measured         = true,
			.minMeasuredValue = 0,
			.maxMeasuredValue = 1'000'000'000'000'000, // 1 million Mwh
			.accuracyRanges =
					DataModel::List<const ElectricalEnergyMeasurement::Structs::MeasurementAccuracyRangeStruct::Type>(energyAccuracyRanges)
	};

	ElectricalEnergyMeasurement::Structs::CumulativeEnergyResetStruct::Type resetStruct = {
			.importedResetTimestamp = MakeOptional(MakeNullable(static_cast<uint32_t>(0))),
			.exportedResetTimestamp = MakeOptional(MakeNullable(static_cast<uint32_t>(0))),
			.importedResetSystime   = MakeOptional(MakeNullable(static_cast<uint64_t>(0))),
			.exportedResetSystime   = MakeOptional(MakeNullable(static_cast<uint64_t>(0))),
	};


	if (gEEMAttrAccess)
	{
		gEEMAttrAccess->Init();

		ElectricalEnergyMeasurement::SetMeasurementAccuracy(endpoint, accuracy);
		ElectricalEnergyMeasurement::SetCumulativeReset(endpoint, MakeNullable(resetStruct));
	}
	return CHIP_NO_ERROR;
}
CHIP_ERROR InitPowerTopology(EndpointId endpoint)
{
	CHIP_ERROR err;

	if (gPTDelegate || gPTInstance)
	{
		ChipLogError(AppServer, "PowerTopology Instance or Delegate already exist.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	gPTDelegate = std::make_unique<PowerTopologyDelegate>();
	if (!gPTDelegate)
	{
		ChipLogError(AppServer, "Failed to allocate memory for PowerTopology Delegate");
		return CHIP_ERROR_NO_MEMORY;
	}

	gPTInstance = std::make_unique<PowerTopologyInstance>(
			EndpointId(endpoint), *gPTDelegate, BitMask<PowerTopology::Feature, uint32_t>(PowerTopology::Feature::kNodeTopology));

	if (!gPTInstance)
	{
		ChipLogError(AppServer, "Failed to allocate memory for PowerTopology Instance");
		gPTDelegate.reset();
		return CHIP_ERROR_NO_MEMORY;
	}

	err = gPTInstance->Init(); /* Register Attribute & Command handlers */
	if (err != CHIP_NO_ERROR)
	{
		ChipLogError(AppServer, "Init failed on gPTInstance");
		gPTInstance.reset();
		gPTDelegate.reset();
		return err;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR InitPowerSource(EndpointId endpoint)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;
	Protocols::InteractionModel::Status status;


	status = PowerSource::Attributes::Status::Set(endpoint, PowerSourceStatusEnum::kActive);
	if(status != Protocols::InteractionModel::Status::Success) {
		ret = CHIP_ERROR_INTERNAL;
		return ret;
	}

	status = PowerSource::Attributes::FeatureMap::Set(endpoint, static_cast<uint32_t>(PowerSource::Feature::kWired));
	if(status != Protocols::InteractionModel::Status::Success) {
		ret = CHIP_ERROR_INTERNAL;
		return ret;
	}

	status = PowerSource::Attributes::WiredNominalVoltage::Set(endpoint, 230'000); // 230V in mv
	if(status != Protocols::InteractionModel::Status::Success) {
		ret = CHIP_ERROR_INTERNAL;
		return ret;
	}

	status = PowerSource::Attributes::WiredMaximumCurrent::Set(endpoint, 32'000); // 32A in mA
	if(status != Protocols::InteractionModel::Status::Success) {
		ret = CHIP_ERROR_INTERNAL;
		return ret;
	}

	status = PowerSource::Attributes::WiredCurrentType::Set(endpoint, PowerSource::WiredCurrentTypeEnum::kAc);
	if(status != Protocols::InteractionModel::Status::Success) {
		ret = CHIP_ERROR_INTERNAL;
		return ret;
	}

	status = PowerSource::Attributes::Description::Set(endpoint, CharSpan::fromCharString("Solar Power Device"));
	if(status != Protocols::InteractionModel::Status::Success) {
		ret = CHIP_ERROR_INTERNAL;
		return ret;
	}

	chip::EndpointId endpointArray[] = { endpoint };
	Span<EndpointId> endpointList    = Span<EndpointId>(endpointArray);
	PowerSourceServer::Instance().SetEndpointList(endpoint, endpointList);

	return ret;
}
}
