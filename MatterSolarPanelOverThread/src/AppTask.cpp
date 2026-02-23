/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "SolarPowerManager.h"
#include "AppConfig.h"
#include "AppEvent.h"
#include "LEDWidget.h"

#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <assert.h>
#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include "FreeRTOS.h"
#include "queue.h"

//#include "DataModelTypes.h"
//#include "PlatformManager.h"
#include "Accessors.h"
#include <lib/support/CHIPMem.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "timers.h"


using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Silabs;
namespace {

EndpointId solar_power_endpoint = 1;

constexpr uint32_t update_period_seconds = 1;
LEDWidget sAppLed;

#define APP_FUNCTION_BUTTON 0
#define APP_SWITCH_BUTTON 1

#if defined(SL_CATALOG_SIMPLE_LED_LED1_PRESENT)
constexpr uint8_t kAppLedId = 1;
#else
constexpr uint8_t kAppLedId = 0;
#endif

} // namespace

AppTask AppTask::sAppTask;
AppTask::WeatherCondition AppTask::sWeather = AppTask::WeatherCondition::kClear;
TimerHandle_t AppTask::sGenerationTimer = nullptr;
CHIP_ERROR AppTask::AppInit()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    chip::DeviceLayer::Silabs::GetPlatform().SetButtonsCb(AppTask::ButtonEventHandler);

    DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t ctx) {
        CHIP_ERROR initErr = SolarPowerManager::Init(static_cast<EndpointId>(ctx));
        if (initErr != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "SolarPowerManager::Init failed: %s", ErrorStr(initErr));
            appError(initErr);
        }
    }, static_cast<intptr_t>(solar_power_endpoint));

    sAppLed.Init(kAppLedId);
    sAppLed.Set(sWeather == WeatherCondition::kClear);

    //srand(1);

    // Update the LCD with the Stored value. Show QR Code if not provisioned
#ifdef DISPLAY_ENABLED
    //GetLCD().WriteDemoUI(LightMgr().IsLightOn());
#ifdef QR_CODE_ENABLED
#ifdef SL_WIFI
    if (!ConnectivityMgr().IsWiFiStationProvisioned())
#else
    if (!ConnectivityMgr().IsThreadProvisioned())
#endif /* !SL_WIFI */
    {
        GetLCD().ShowQRCode(true);
    }
#endif // QR_CODE_ENABLED
#endif


#ifdef DISPLAY_ENABLED

#ifdef QR_CODE_ENABLED
    if (BaseApplication::GetProvisionStatus())
    {
        GetLCD().ShowQRCode(true);
    }
#endif // QR_CODE_ENABLED

#endif // DISPLAY_ENABLED
    sGenerationTimer = xTimerCreate("SolarGen",
                                    pdMS_TO_TICKS(update_period_seconds * 1000),
                                    pdTRUE /* auto-reload */,
                                    nullptr,
                                    GenerationTimerCallback);
    if (sGenerationTimer == nullptr)
    {
        err = APP_ERROR_CREATE_TIMER_FAILED;
        appError(err);
        return err;
    }
    xTimerStart(sGenerationTimer, 0);

    BaseApplication::InitCompleteCallback(err);
    return err;
}

CHIP_ERROR AppTask::StartAppTask()
{
    return BaseApplication::StartAppTask(AppTaskMain);
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;
    osMessageQueueId_t sAppEventQueue = *(static_cast<osMessageQueueId_t *>(pvParameter));

    CHIP_ERROR err = sAppTask.Init();
    if (err != CHIP_NO_ERROR)
    {
        SILABS_LOG("AppTask.Init() failed");
        appError(err);
    }

#if !(defined(CHIP_CONFIG_ENABLE_ICD_SERVER) && CHIP_CONFIG_ENABLE_ICD_SERVER)
    sAppTask.StartStatusLEDTimer();
#endif

    SILABS_LOG("App Task started");

    while (true)
    {
        osStatus_t eventReceived = osMessageQueueGet(sAppEventQueue, &event, nullptr, osWaitForever);
        while (eventReceived == osOK)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = osMessageQueueGet(sAppEventQueue, &event, nullptr, 0);
        }
    }
}

void AppTask::ButtonEventHandler(uint8_t button, uint8_t btnAction)
{
    AppEvent button_event           = {};
    button_event.Type               = AppEvent::kEventType_Button;
    button_event.ButtonEvent.Action = btnAction;

    if (button == APP_SWITCH_BUTTON && btnAction == static_cast<uint8_t>(SilabsPlatform::ButtonAction::ButtonPressed))
    {
        button_event.Handler = ProccessButtonEvent;
        AppTask::GetAppTask().PostEvent(&button_event);
    }
    else if (button == APP_FUNCTION_BUTTON)
    {
        button_event.Handler = BaseApplication::ButtonHandler;
        AppTask::GetAppTask().PostEvent(&button_event);
    }
}

enum CurrentLevels {
	NONE = 0,
	LOW = 2000,
	HIGH = 15000
};

// Base “clear sky” generation (mA). Cloudy will be 30-40% of this. Night is 0.
static constexpr int kClearCurrent_mA = 15000;

static int sCurrentNoisy_mA = 0;
static int sVoltage_V       = 230;
static int sPower_mW        = 0;

static uint64_t sCumulativeEnergy_mWh = 0;

void AppTask::ProccessButtonEvent(AppEvent* event)
{
	VerifyOrReturn(event->Type == AppEvent::kEventType_Button);

	// Cycle weather conditions on APP_SWITCH_BUTTON press.
    switch (sWeather)
    {
    case WeatherCondition::kClear:
        sWeather = WeatherCondition::kCloudy;
        break;
    case WeatherCondition::kCloudy:
        sWeather = WeatherCondition::kNight;
        break;
    case WeatherCondition::kNight:
    default:
        sWeather = WeatherCondition::kClear;
        break;
    }

    // LED is ON only for Clear conditions.
    sAppLed.Set(sWeather == WeatherCondition::kClear);
}

void AppTask::GenerationTimerCallback(TimerHandle_t)
{
    AppEvent timer_event;
    timer_event.Type               = AppEvent::kEventType_Timer;
    timer_event.TimerEvent.Context = nullptr;
    timer_event.Handler            = GenerationTickEventHandler;
    AppTask::GetAppTask().PostEvent(&timer_event);
}

void AppTask::GenerationTickEventHandler(AppEvent * event)
{
    VerifyOrReturn(event->Type == AppEvent::kEventType_Timer);
    // Compute “ideal” current from weather (outside CHIP stack).
    int baseCurrent_mA = 0;
    switch (sWeather)
    {
    case WeatherCondition::kClear:
        baseCurrent_mA = kClearCurrent_mA;
        break;
    case WeatherCondition::kCloudy:
        // 30-40% of clear
        baseCurrent_mA = (kClearCurrent_mA * (30 + (rand() % 11))) / 100;
        break;
    case WeatherCondition::kNight:
    default:
        baseCurrent_mA = 0;
        break;
    }

    // Add small noise (+/-5%) to make readings feel “live”.
    const int noisePermille = (static_cast<int>(rand() % 101) - 50); // -50..+50
    sCurrentNoisy_mA        = baseCurrent_mA + (baseCurrent_mA * noisePermille) / 1000;

    sPower_mW = sVoltage_V * sCurrentNoisy_mA;

    const uint64_t periodic_mWh = static_cast<uint64_t>(sPower_mW) * update_period_seconds / 3600;
    sCumulativeEnergy_mWh += periodic_mWh;

    // Transfer readings to the CHIP thread (and into Matter clusters).
    auto * readings = chip::Platform::New<GeneratedReadings>();
    if (readings == nullptr)
    {
        return;
    }
    readings->voltage_V             = sVoltage_V;
    readings->current_mA            = sCurrentNoisy_mA;
    readings->power_mW              = sPower_mW;
    readings->periodic_energy_mWh   = periodic_mWh;
    readings->cumulative_energy_mWh = sCumulativeEnergy_mWh;

    DeviceLayer::PlatformMgr().ScheduleWork(ApplyMatterReadings, reinterpret_cast<intptr_t>(readings));

#ifdef DISPLAY_ENABLED
	AppTask::GetAppTask().UpdateDisplay();
#endif // DISPLAY_ENABLED
}

void AppTask::ApplyMatterReadings(intptr_t context)
{
    auto * readings = reinterpret_cast<GeneratedReadings *>(context);
    VerifyOrReturn(readings != nullptr);

    // All Matter / cluster interaction happens on the CHIP thread.
    SolarPowerManager::UpdatePowerReading(readings->voltage_V, readings->current_mA, readings->power_mW);
    SolarPowerManager::SendPeriodicEnergyReading(readings->periodic_energy_mWh);
    SolarPowerManager::SendCumulativeEnergyReading(readings->cumulative_energy_mWh);

    chip::Platform::Delete(readings);
}

#ifdef DISPLAY_ENABLED
void AppTask::UpdateDisplay()
{
#if DEBUG_CURRENT_MA_ON_DISPLAY
	SilabsLCD::DisplayStatus_t lcd_status = {};
	itoa(sCurrentNoisy_mA, lcd_status.networkName, 10);

	SilabsLCD& lcd = GetLCD();
	lcd.SetStatus(lcd_status);
	lcd.WriteStatus();
#endif
}
#endif // DISPLAY_ENABLED
