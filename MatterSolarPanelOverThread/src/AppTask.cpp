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

#include <app/clusters/mode-select-server/supported-modes-manager.h>
#include <app/data-model/List.h>

#include "FreeRTOS.h"
#include "queue.h"

//#include "DataModelTypes.h"
//#include "PlatformManager.h"
#include "Accessors.h"
#include <lib/support/CHIPMem.h>
#include <lib/shell/Engine.h>
#include <cstring>
#include <cstdlib>

// FreeRTOS
#include "FreeRTOS.h"
#include "timers.h"


using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Silabs;


#ifdef DISPLAY_ENABLED
#define LCD_CTX() \
    static_cast<GLIB_Context_t *>(AppTask::GetLCD().Context())

#define APP_BITMAP_WIDTH 64
#define APP_BITMAP_HEIGHT 64

#define APP_X_POSITION ((LCD_CTX()->pDisplayGeometry->xSize - APP_BITMAP_WIDTH) / 2)
#define APP_Y_POSITION (LCD_CTX()->pDisplayGeometry->ySize - APP_BITMAP_HEIGHT - 5)

#define PROT2_X_POSITION 104
#define PROT2_Y_POSITION (APP_Y_POSITION + (APP_Y_POSITION / 2))

#define THREAD_BITMAP_WIDTH 16
#define THREAD_BITMAP_HEIGHT 18
#define PROT1_BITMAP_WIDTH THREAD_BITMAP_WIDTH
#define PROT1_BITMAP_HEIGHT THREAD_BITMAP_HEIGHT
#define PROT1_X_POSITION 8
#define PROT1_Y_POSITION (APP_Y_POSITION + (APP_Y_POSITION / 2))

#define MATTER_LOGO_WIDTH 18
#define MATTER_LOGO_HEIGHT 17

#define SILICONLABS_BITMAP_WIDTH 128
#define SILICONLABS_BITMAP_HEIGHT 45

#define SILABS_BITMAP                                                                                                              \
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xf0, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0xfd, 0xff, 0xff, 0x01, 0x00, 0x00, 0xc0, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0x3f, 0x00,    \
        0x00, 0x00, 0x00, 0xff, 0xff, 0x1f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0xfe,    \
        0xff, 0x1f, 0xf8, 0xff, 0xff, 0xff, 0x7f, 0xfc, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x1f, 0xf0, 0xff,    \
        0xff, 0xff, 0x1f, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x1f, 0xe0, 0xff, 0xff, 0xff, 0xc3, 0xff,    \
        0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x0f, 0xc0, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x07, 0x00, 0x00,    \
        0x00, 0x00, 0x00, 0xff, 0xff, 0x07, 0xc0, 0xff, 0xff, 0x1f, 0xf8, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,    \
        0xff, 0x01, 0xc0, 0xff, 0xff, 0x07, 0xfc, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x7f, 0x00, 0x80, 0xff,    \
        0xff, 0x01, 0xfe, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x80, 0xff, 0x7f, 0x00, 0xff, 0xff,    \
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x3f, 0x80, 0xff, 0xff, 0xff, 0x00, 0x00, 0xf0,    \
        0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x0f, 0x80, 0xff, 0xff, 0xff, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x01,    \
        0x00, 0x00, 0xc0, 0xff, 0x03, 0xc0, 0xff, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x00, 0xe0, 0xff,    \
        0x01, 0xc0, 0xff, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xe0, 0xff, 0x00, 0xc0, 0xff, 0xff,    \
        0xff, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0xf0, 0x7f, 0x00, 0x80, 0xff, 0xff, 0xff, 0x0f, 0xfc, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0xf8, 0x3f, 0x00, 0x80, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff,    \
        0x0f, 0x00, 0xfc, 0x1f, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xc0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0xfe, 0x0f,    \
        0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0x1f, 0x00, 0xff, 0x07, 0x00, 0x00, 0xf0, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0x3f, 0xfc, 0xff, 0xff, 0x1f, 0x80, 0xff, 0x07, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0x3f, 0xf8, 0xff, 0xff, 0x1f, 0xc0, 0xff, 0x03, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x00, 0xf0, 0xff, 0xff,    \
        0x0f, 0xf0, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x3f, 0x00, 0x00, 0xf0, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0x03,    \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0x07, 0xfe, 0xff, 0x01, 0x00, 0x00, 0x3f, 0x00,    \
        0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x83, 0xff, 0xff, 0x01, 0x00, 0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00,    \
        0x00, 0xfc, 0xff, 0xff, 0xe1, 0xff, 0xff, 0x03, 0x00, 0xf0, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff,    \
        0xf8, 0xff, 0xff, 0x03, 0x00, 0xf8, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0x03,    \
        0x00, 0xfe, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xdf, 0xff, 0xff, 0xff, 0x07, 0x00, 0xff, 0xff, 0x01,    \
        0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x80, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,    \
        0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xc0, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff,    \
        0xff, 0xff, 0xff, 0x3f, 0xc0, 0xff, 0xff, 0x1f, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0xc0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x81, 0xff, 0xff, 0xff,    \
        0x1f, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

#define THREAD_BITMAP                                                                                                              \
    0x1F, 0xF8, 0x07, 0xE0, 0x03, 0xCE, 0x01, 0x9F, 0x01, 0xB3, 0x00, 0x33, 0xF0, 0x3F, 0xF8, 0x1F, 0x0C, 0x03, 0x0C, 0x03, 0x18,  \
        0x03, 0x11, 0x83, 0x01, 0x83, 0x03, 0xC3, 0x07, 0xE3, 0x1F, 0xFB, 0x7f, 0xff, 0xff, 0xff

#define MATTER_LOGO_BITMAP                                                                                                         \
    0xFF, 0xFF, 0xFF, 0xF3, 0xFF, 0xCF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFC, 0x3F, 0x12, 0xFF, 0x01, 0xFE, 0xFF, 0xFF, 0xF3, 0x3F, 0x8F,  \
        0x7F, 0xFC, 0xFC, 0xFC, 0xE3, 0xF1, 0x87, 0x87, 0xC7, 0xDE, 0x88, 0x33, 0xC7, 0xCF, 0xFC, 0xFF, 0xFF, 0x03

#define SILICONLABS_X_POSITION ((LCD_CTX()->pDisplayGeometry->xSize - SILICONLABS_BITMAP_WIDTH) / 2)
#define SILICONLABS_Y_POSITION 0

#endif // DISPLAY_ENABLED



namespace {


using SolarModeOption = ModeSelect::Structs::ModeOptionStruct::Type;
using SolarSemanticTag = ModeSelect::Structs::SemanticTagStruct::Type;
using Status = chip::Protocols::InteractionModel::Status;
using chip::Shell::Engine;
using chip::Shell::shell_command_t;

constexpr EndpointId solar_power_endpoint = 1;
constexpr EndpointId weather_endpoint = 2;

// Solar demo defaults: edit these together when tuning the simulated session.
constexpr uint32_t kUpdatePeriodSeconds = 1;
constexpr uint32_t kEnergyReportPeriodSeconds = 10; // Keep power live without emitting energy events every second.
static_assert(kEnergyReportPeriodSeconds >= kUpdatePeriodSeconds, "Energy reporting must not outpace sampling");
constexpr uint32_t kProfileRampSeconds = 1 * 60; // Dawn-to-peak and peak-to-dusk make a ~2 minute session.
constexpr uint32_t kNominalPeakPower_W = 4000;
constexpr uint32_t kDawnMinPower_W = 400;
constexpr uint32_t kDawnMaxPower_W = 1100;
constexpr uint32_t kDuskMinPower_W = 400;
constexpr uint32_t kDuskMaxPower_W = 1100;
constexpr uint32_t kPeakMinPower_W = 3700;
constexpr uint32_t kPeakMaxPower_W = kNominalPeakPower_W;
constexpr uint16_t kClearMultiplierPermille = 1000;
constexpr uint16_t kOvercastMultiplierPermille = 400;
constexpr uint32_t kDipCheckIntervalSeconds = 30;
constexpr uint16_t kClearDipProbabilityPermille = 50;    // 5% each check
constexpr uint16_t kOvercastDipProbabilityPermille = 800; // 80% each check
constexpr uint16_t kClearDipMultiplierPermille = 800;     // 80% of weather-adjusted output
constexpr uint16_t kOvercastDipMultiplierPermille = 150;  // 15% of weather-adjusted output
constexpr uint32_t kDipHoldMinSeconds = 10;
constexpr uint32_t kDipHoldMaxSeconds = 30;
constexpr uint32_t kDipRampMinSeconds = 1;
constexpr uint32_t kDipRampMaxSeconds = 3;

uint32_t RandomBetween(uint32_t minimum, uint32_t maximum)
{
    return minimum + static_cast<uint32_t>(std::rand()) % (maximum - minimum + 1);
}

const SolarModeOption kDayProfileModes[] = {
    {
        .label = "Dawn"_span,
        .mode = static_cast<uint8_t>(AppTask::DayProfile::kDawn),
        .semanticTags = DataModel::List<const SolarSemanticTag>{},
    },
    {
        .label = "Peak"_span,
        .mode = static_cast<uint8_t>(AppTask::DayProfile::kPeak),
        .semanticTags = DataModel::List<const SolarSemanticTag>{},
    },
    {
        .label = "Dusk"_span,
        .mode = static_cast<uint8_t>(AppTask::DayProfile::kDusk),
        .semanticTags = DataModel::List<const SolarSemanticTag>{},
    },
    {
        .label = "Night"_span,
        .mode = static_cast<uint8_t>(AppTask::DayProfile::kNight),
        .semanticTags = DataModel::List<const SolarSemanticTag>{},
    },
};

const SolarModeOption kWeatherModes[] = {
    {
        .label = "Clear"_span,
        .mode = static_cast<uint8_t>(AppTask::WeatherCondition::kClear),
        .semanticTags = DataModel::List<const SolarSemanticTag>{},
    },
    {
        .label = "Overcast"_span,
        .mode = static_cast<uint8_t>(AppTask::WeatherCondition::kOvercast),
        .semanticTags = DataModel::List<const SolarSemanticTag>{},
    },
};

class SolarSupportedModesManager final : public ModeSelect::SupportedModesManager
{
public:
    ModeOptionsProvider getModeOptionsProvider(EndpointId endpointId) const override
    {
        if (endpointId == solar_power_endpoint)
            return { kDayProfileModes, kDayProfileModes + 4 };
        if (endpointId == weather_endpoint)
            return { kWeatherModes, kWeatherModes + 2 };
        return {};
    }

    Status getModeOptionByMode(
        EndpointId endpointId,
        uint8_t mode,
        const SolarModeOption ** option) const override
    {
        if (option == nullptr)
        {
            return Status::Failure;
        }

        *option = nullptr;

        const SolarModeOption * begin = nullptr;
        const SolarModeOption * end = nullptr;
        if (endpointId == solar_power_endpoint)
        {
            begin = kDayProfileModes;
            end = kDayProfileModes + 4;
        }
        else if (endpointId == weather_endpoint)
        {
            begin = kWeatherModes;
            end = kWeatherModes + 2;
        }
        else
        {
            return Status::UnsupportedCluster;
        }

        for (const SolarModeOption * candidate = begin; candidate != end; ++candidate)
        {
            if (candidate->mode == mode)
            {
                *option = candidate;
                return Status::Success;
            }
        }

        return Status::InvalidCommand;
    }
};

SolarSupportedModesManager gSolarSupportedModesManager;

CHIP_ERROR SolarModeCommand(int argc, char ** argv)
{
    if (argc != 1)
    {
        ChipLogError(Shell, "Usage: solar-mode <dawn|peak|dusk|night>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t mode;
    if (std::strcmp(argv[0], "dawn") == 0)
    {
        mode = static_cast<uint8_t>(AppTask::DayProfile::kDawn);
    }
    else if (std::strcmp(argv[0], "peak") == 0)
    {
        mode = static_cast<uint8_t>(AppTask::DayProfile::kPeak);
    }
    else if (std::strcmp(argv[0], "dusk") == 0)
    {
        mode = static_cast<uint8_t>(AppTask::DayProfile::kDusk);
    }
    else if (std::strcmp(argv[0], "night") == 0)
    {
        mode = static_cast<uint8_t>(AppTask::DayProfile::kNight);
    }
    else
    {
        ChipLogError(Shell, "Unknown day profile '%s'; use dawn, peak, dusk, or night", argv[0]);
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    AppTask::PostDayProfileMode(mode, true);
    return CHIP_NO_ERROR;
}

CHIP_ERROR SolarWeatherCommand(int argc, char ** argv)
{
    if (argc != 1 || (std::strcmp(argv[0], "clear") != 0 && std::strcmp(argv[0], "overcast") != 0))
    {
        ChipLogError(Shell, "Usage: solar-weather <clear|overcast>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    const uint8_t mode = (std::strcmp(argv[0], "clear") == 0)
        ? static_cast<uint8_t>(AppTask::WeatherCondition::kClear)
        : static_cast<uint8_t>(AppTask::WeatherCondition::kOvercast);
    AppTask::PostWeatherMode(mode, true);
    return CHIP_NO_ERROR;
}

const shell_command_t kSolarModeCommand = {
    &SolarModeCommand,
    "solar-mode",
    "Set solar day profile: solar-mode <dawn|peak|dusk|night>",
};

const shell_command_t kSolarWeatherCommand = {
    &SolarWeatherCommand,
    "solar-weather",
    "Set solar weather: solar-weather <clear|overcast>",
};

LEDWidget sAppLed;

#define APP_FUNCTION_BUTTON 0
#define APP_SWITCH_BUTTON 1

#if defined(SL_CATALOG_SIMPLE_LED_LED1_PRESENT)
constexpr uint8_t kAppLedId = 1;
#else
constexpr uint8_t kAppLedId = 0;
#endif

#ifdef DISPLAY_ENABLED
// Custom UI callback used by SilabsLCD::WriteDemoUI().
static const uint8_t siliconlabsBitmap[] = { SILABS_BITMAP };
static const uint8_t matterLogoBitmap[]  = { MATTER_LOGO_BITMAP };

static const uint8_t networkBitMap[] = { THREAD_BITMAP };

void DrawCustomDemoUI(GLIB_Context_t * context)
{
    uint8_t lineNb = 5;
    char str[24];

    


    GLIB_clear(LCD_CTX());
    GLIB_drawBitmap(LCD_CTX(), SILICONLABS_X_POSITION, SILICONLABS_Y_POSITION, SILICONLABS_BITMAP_WIDTH,
                    SILICONLABS_BITMAP_HEIGHT, siliconlabsBitmap);
    GLIB_drawBitmap(LCD_CTX(), PROT1_X_POSITION, PROT1_Y_POSITION, PROT1_BITMAP_WIDTH, PROT1_BITMAP_HEIGHT, networkBitMap);
    GLIB_drawBitmap(LCD_CTX(), PROT2_X_POSITION, PROT2_Y_POSITION, MATTER_LOGO_WIDTH, MATTER_LOGO_HEIGHT, matterLogoBitmap);
    
    
    sprintf(str, "Solar Panel");
    GLIB_drawStringOnLine(LCD_CTX(), str, lineNb++, GLIB_ALIGN_CENTER, 0, 0, true);
    lineNb++; //empty line
    snprintf(str, sizeof(str), "Day:");
    GLIB_drawStringOnLine(LCD_CTX(), str, lineNb++, GLIB_ALIGN_CENTER, 0, 0, true);
    snprintf(str, sizeof(str), "%s", AppTask::DayProfileToString(AppTask::GetDayProfile()));
    GLIB_drawStringOnLine(LCD_CTX(), str, lineNb++, GLIB_ALIGN_CENTER, 0, 0, true);
    snprintf(str, sizeof(str), "Weather:");
    GLIB_drawStringOnLine(LCD_CTX(), str, lineNb++, GLIB_ALIGN_CENTER, 0, 0, true);
    snprintf(str, sizeof(str), "%s", AppTask::WeatherToString(AppTask::GetWeather()));
    GLIB_drawStringOnLine(LCD_CTX(), str, lineNb++, GLIB_ALIGN_CENTER, 0, 0, true);

    updateDisplay();
}
#endif
} // namespace

AppTask AppTask::sAppTask;
AppTask::DayProfile AppTask::sDayProfile = AppTask::DayProfile::kPeak;
AppTask::WeatherCondition AppTask::sWeather = AppTask::WeatherCondition::kClear;
TimerHandle_t AppTask::sGenerationTimer = nullptr;

namespace {

uint32_t sProfilePower_W = 0;
uint32_t sSelectedProfilePower_W = 0;
uint32_t sProfileRampStart_W = 0;
uint32_t sProfileRampTarget_W = 0;
uint32_t sProfileRampElapsedSeconds = 0;
uint32_t sDipCheckElapsedSeconds = 0;
uint32_t sDipStageElapsedSeconds = 0;
uint32_t sDipStageDurationSeconds = 0;
uint32_t sDipHoldRemainingSeconds = 0;
uint16_t sDipMultiplierPermille = 1000;
uint16_t sDipTargetMultiplierPermille = 1000;
uint64_t sEnergyRemainder_mWSeconds = 0;
uint64_t sPendingPeriodicEnergy_mWh = 0;
uint32_t sEnergyReportElapsedSeconds = 0;

enum class DipStage : uint8_t
{
    kWaiting,
    kRampingDown,
    kHolding,
    kRampingUp,
};
DipStage sDipStage = DipStage::kWaiting;

uint32_t GetProfilePower_W(AppTask::DayProfile profile)
{
    switch (profile)
    {
    case AppTask::DayProfile::kDawn:
        return RandomBetween(kDawnMinPower_W, kDawnMaxPower_W);
    case AppTask::DayProfile::kPeak:
        return RandomBetween(kPeakMinPower_W, kPeakMaxPower_W);
    case AppTask::DayProfile::kDusk:
        return RandomBetween(kDuskMinPower_W, kDuskMaxPower_W);
    case AppTask::DayProfile::kNight:
    default:
        return 0;
    }
}

uint32_t GetWeatherAdjustedProfilePower_W()
{
    const uint32_t multiplier = AppTask::GetWeather() == AppTask::WeatherCondition::kClear
        ? kClearMultiplierPermille
        : kOvercastMultiplierPermille;
    return (sSelectedProfilePower_W * multiplier) / 1000;
}

void StartProfileRamp(bool selectNewDayProfile)
{
    if (selectNewDayProfile)
    {
        sSelectedProfilePower_W = GetProfilePower_W(AppTask::GetDayProfile());
    }
    sProfileRampStart_W = sProfilePower_W;
    sProfileRampTarget_W = GetWeatherAdjustedProfilePower_W();
    sProfileRampElapsedSeconds = 0;
}

void StartCloudDip()
{
    const bool clear = AppTask::GetWeather() == AppTask::WeatherCondition::kClear;
    const uint16_t probability = clear ? kClearDipProbabilityPermille : kOvercastDipProbabilityPermille;
    if (RandomBetween(0, 999) >= probability)
    {
        return;
    }

    sDipTargetMultiplierPermille = clear ? kClearDipMultiplierPermille : kOvercastDipMultiplierPermille;
    sDipStage = DipStage::kRampingDown;
    sDipStageDurationSeconds = RandomBetween(kDipRampMinSeconds, kDipRampMaxSeconds);
    sDipStageElapsedSeconds = 0;
}

void AdvanceCloudDip()
{
    if (AppTask::GetDayProfile() == AppTask::DayProfile::kNight || sSelectedProfilePower_W == 0)
    {
        sDipStage = DipStage::kWaiting;
        sDipCheckElapsedSeconds = 0;
        sDipMultiplierPermille = 1000;
        return;
    }

    switch (sDipStage)
    {
    case DipStage::kWaiting:
        if (++sDipCheckElapsedSeconds >= kDipCheckIntervalSeconds)
        {
            sDipCheckElapsedSeconds = 0;
            StartCloudDip();
        }
        break;
    case DipStage::kRampingDown:
        if (++sDipStageElapsedSeconds >= sDipStageDurationSeconds)
        {
            sDipMultiplierPermille = sDipTargetMultiplierPermille;
            sDipHoldRemainingSeconds = RandomBetween(kDipHoldMinSeconds, kDipHoldMaxSeconds);
            sDipStage = DipStage::kHolding;
        }
        else
        {
            const uint32_t progress = sDipStageElapsedSeconds;
            const uint32_t duration = sDipStageDurationSeconds;
            sDipMultiplierPermille = static_cast<uint16_t>(1000 - ((1000 - sDipTargetMultiplierPermille) * progress) / duration);
        }
        break;
    case DipStage::kHolding:
        if (sDipHoldRemainingSeconds > 0 && --sDipHoldRemainingSeconds == 0)
        {
            sDipStage = DipStage::kRampingUp;
            sDipStageElapsedSeconds = 0;
            sDipStageDurationSeconds = RandomBetween(kDipRampMinSeconds, kDipRampMaxSeconds);
        }
        break;
    case DipStage::kRampingUp:
        if (++sDipStageElapsedSeconds >= sDipStageDurationSeconds)
        {
            sDipMultiplierPermille = 1000;
            sDipStage = DipStage::kWaiting;
            sDipCheckElapsedSeconds = 0;
        }
        else
        {
            const uint32_t progress = sDipStageElapsedSeconds;
            const uint32_t duration = sDipStageDurationSeconds;
            sDipMultiplierPermille = static_cast<uint16_t>(sDipTargetMultiplierPermille + ((1000 - sDipTargetMultiplierPermille) * progress) / duration);
        }
        break;
    }
}

} // namespace

CHIP_ERROR AppTask::AppInit()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    ModeSelect::setSupportedModesManager(&gSolarSupportedModesManager);
    Engine::Root().RegisterCommands(&kSolarModeCommand, 1);
    Engine::Root().RegisterCommands(&kSolarWeatherCommand, 1);
    chip::DeviceLayer::Silabs::GetPlatform().SetButtonsCb(AppTask::ButtonEventHandler);

    StartProfileRamp(true);
    err = DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {
        const Status dayStatus = ModeSelect::Attributes::CurrentMode::Set(solar_power_endpoint,
            static_cast<uint8_t>(DayProfile::kPeak));
        const Status weatherStatus = ModeSelect::Attributes::CurrentMode::Set(weather_endpoint,
            static_cast<uint8_t>(WeatherCondition::kClear));
        if (dayStatus != Status::Success || weatherStatus != Status::Success)
        {
            ChipLogError(AppServer, "Unable to initialize solar Mode Select attributes (day=%u, weather=%u)",
                         to_underlying(dayStatus), to_underlying(weatherStatus));
        }
    }, 0);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Failed to schedule initial Mode Select values: %s", ErrorStr(err));
        return err;
    }

    err = DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t ctx) {
        CHIP_ERROR initErr = SolarPowerManager::Init(static_cast<EndpointId>(ctx));
        if (initErr != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "SolarPowerManager::Init failed: %s", ErrorStr(initErr));
            appError(initErr);
        }
    }, static_cast<intptr_t>(solar_power_endpoint));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Failed to schedule SolarPowerManager initialization: %s", ErrorStr(err));
        return err;
    }

    sAppLed.Init(kAppLedId);
    sAppLed.Set(sWeather == WeatherCondition::kClear);

    
#ifdef DISPLAY_ENABLED
    // Install a custom UI renderer for the Demo screen.
    GetLCD().SetCustomUI(DrawCustomDemoUI);

    RequestDemoScreenRefresh();

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
#endif // DISPLAY_ENABLED
    sGenerationTimer = xTimerCreate("SolarGen",
                                    pdMS_TO_TICKS(kUpdatePeriodSeconds * 1000),
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

static int sCurrent_mA = 0;
static int sVoltage_V       = 230;
static int sPower_mW        = 0;

static uint64_t sCumulativeEnergy_mWh = 0;

void AppTask::ProccessButtonEvent(AppEvent* event)
{
	VerifyOrReturn(event->Type == AppEvent::kEventType_Button);

	// PB1 toggles the weather mode independently of the day profile.
    const WeatherCondition nextWeather = sWeather == WeatherCondition::kClear
        ? WeatherCondition::kOvercast
        : WeatherCondition::kClear;
    PostWeatherMode(static_cast<uint8_t>(nextWeather), true);
}

void AppTask::PostDayProfileMode(uint8_t mode, bool updateMatterAttribute)
{
    AppEvent event = {};
    event.Type = AppEvent::kEventType_ModeSelect;
    event.ModeSelectEvent.Endpoint = solar_power_endpoint;
    event.ModeSelectEvent.Mode = mode;
    event.ModeSelectEvent.UpdateMatterAttribute = updateMatterAttribute;
    event.Handler = ModeSelectEventHandler;
    GetAppTask().PostEvent(&event);
}

void AppTask::PostWeatherMode(uint8_t mode, bool updateMatterAttribute)
{
    AppEvent event = {};
    event.Type = AppEvent::kEventType_ModeSelect;
    event.ModeSelectEvent.Endpoint = weather_endpoint;
    event.ModeSelectEvent.Mode = mode;
    event.ModeSelectEvent.UpdateMatterAttribute = updateMatterAttribute;
    event.Handler = ModeSelectEventHandler;
    GetAppTask().PostEvent(&event);
}

void AppTask::ModeSelectEventHandler(AppEvent * event)
{
    VerifyOrReturn(event != nullptr && event->Type == AppEvent::kEventType_ModeSelect);

    const EndpointId endpoint = event->ModeSelectEvent.Endpoint;
    const uint8_t mode = event->ModeSelectEvent.Mode;
    if (endpoint == solar_power_endpoint)
    {
        if (mode > static_cast<uint8_t>(DayProfile::kNight))
        {
            ChipLogError(AppServer, "Ignoring unsupported solar day profile %u", mode);
            return;
        }
        const DayProfile requestedProfile = static_cast<DayProfile>(mode);
        if (sDayProfile != requestedProfile)
        {
            sDayProfile = requestedProfile;
            StartProfileRamp(true);
            GetAppTask().RequestDemoScreenRefresh();
        }
    }
    else if (endpoint == weather_endpoint)
    {
        if (mode > static_cast<uint8_t>(WeatherCondition::kOvercast))
        {
            ChipLogError(AppServer, "Ignoring unsupported solar weather mode %u", mode);
            return;
        }
        const WeatherCondition requestedWeather = static_cast<WeatherCondition>(mode);
        if (sWeather != requestedWeather)
        {
            sWeather = requestedWeather;
            StartProfileRamp(false);
            sAppLed.Set(sWeather == WeatherCondition::kClear);
            GetAppTask().RequestDemoScreenRefresh();
        }
    }
    else
        return;

    if (event->ModeSelectEvent.UpdateMatterAttribute)
    {
        const CHIP_ERROR err = DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t context) {
            const uint32_t packed = static_cast<uint32_t>(context);
            const EndpointId endpointId = static_cast<EndpointId>(packed >> 8);
            const uint8_t requestedMode = static_cast<uint8_t>(packed & 0xFF);
            const Status status = ModeSelect::Attributes::CurrentMode::Set(endpointId, requestedMode);
            if (status != Status::Success)
            {
                ChipLogError(AppServer, "Failed to set ModeSelect CurrentMode on endpoint %u: status %u",
                             endpointId, to_underlying(status));
            }
        }, static_cast<intptr_t>((static_cast<uint32_t>(endpoint) << 8) | mode));
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "Failed to schedule ModeSelect CurrentMode update: %s", ErrorStr(err));
        }
    }
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
    if (sProfileRampElapsedSeconds < kProfileRampSeconds)
    {
        sProfileRampElapsedSeconds = (sProfileRampElapsedSeconds + kUpdatePeriodSeconds < kProfileRampSeconds)
            ? sProfileRampElapsedSeconds + kUpdatePeriodSeconds
            : kProfileRampSeconds;
        const int32_t delta = static_cast<int32_t>(sProfileRampTarget_W) - static_cast<int32_t>(sProfileRampStart_W);
        sProfilePower_W = static_cast<uint32_t>(static_cast<int32_t>(sProfileRampStart_W) +
            (delta * static_cast<int32_t>(sProfileRampElapsedSeconds)) / static_cast<int32_t>(kProfileRampSeconds));
    }

    AdvanceCloudDip();
    const uint32_t power_W = (sProfilePower_W * sDipMultiplierPermille) / 1000;
    sCurrent_mA = (sVoltage_V == 0) ? 0 : static_cast<int>((power_W * 1000 + (sVoltage_V / 2)) / sVoltage_V);
    sPower_mW = sVoltage_V * sCurrent_mA;

    const uint64_t energyNumerator = static_cast<uint64_t>(sPower_mW) * kUpdatePeriodSeconds + sEnergyRemainder_mWSeconds;
    const uint64_t periodic_mWh = energyNumerator / 3600;
    sEnergyRemainder_mWSeconds = energyNumerator % 3600;
    sCumulativeEnergy_mWh += periodic_mWh;
    sPendingPeriodicEnergy_mWh += periodic_mWh;
    sEnergyReportElapsedSeconds += kUpdatePeriodSeconds;

    // Transfer readings to the CHIP thread (and into Matter clusters).
    auto * readings = chip::Platform::New<GeneratedReadings>();
    if (readings == nullptr)
    {
        return;
    }
    readings->voltage_V             = sVoltage_V;
    readings->current_mA            = sCurrent_mA;
    readings->power_mW              = sPower_mW;
    readings->publish_energy       = sEnergyReportElapsedSeconds >= kEnergyReportPeriodSeconds;
    readings->periodic_energy_mWh   = sPendingPeriodicEnergy_mWh;
    readings->cumulative_energy_mWh = sCumulativeEnergy_mWh;

    const CHIP_ERROR err = DeviceLayer::PlatformMgr().ScheduleWork(ApplyMatterReadings, reinterpret_cast<intptr_t>(readings));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Failed to schedule solar readings update: %s", ErrorStr(err));
        chip::Platform::Delete(readings);
    }
    else if (sEnergyReportElapsedSeconds >= kEnergyReportPeriodSeconds)
    {
        // Clear only after successful handoff. Allocation/scheduling failures
        // retain the entire interval for the next attempt.
        sPendingPeriodicEnergy_mWh = 0;
        sEnergyReportElapsedSeconds = 0;
    }
}

void AppTask::ApplyMatterReadings(intptr_t context)
{
    auto * readings = reinterpret_cast<GeneratedReadings *>(context);
    VerifyOrReturn(readings != nullptr);

    // All Matter / cluster interaction happens on the CHIP thread.
    SolarPowerManager::UpdatePowerReading(readings->voltage_V, readings->current_mA, readings->power_mW);
    if (readings->publish_energy)
    {
        SolarPowerManager::SendPeriodicEnergyReading(readings->periodic_energy_mWh);
        SolarPowerManager::SendCumulativeEnergyReading(readings->cumulative_energy_mWh);
    }

    chip::Platform::Delete(readings);
}

const char * AppTask::DayProfileToString(DayProfile profile)
{
    switch (profile)
    {
    case DayProfile::kDawn:
        return "Dawn";
    case DayProfile::kPeak:
        return "Peak";
    case DayProfile::kDusk:
        return "Dusk";
    case DayProfile::kNight:
        return "Night";
    default:
        return "Unknown";
    }
}

const char * AppTask::WeatherToString(WeatherCondition condition)
{
    switch (condition)
    {
    case WeatherCondition::kClear:
        return "Clear";
    case WeatherCondition::kOvercast:
        return "Overcast";
    default:
        return "Unknown";
    }
}

AppTask::DayProfile AppTask::GetDayProfile()
{
    return sDayProfile;
}

AppTask::WeatherCondition AppTask::GetWeather()
{
    return sWeather;
}

#ifdef DISPLAY_ENABLED
void AppTask::UpdateDisplay()
{
    UpdateLCDStatusScreen();
    GetLCD().CycleScreens();  
}
#endif

void AppTask::RequestDemoScreenRefresh()
{
#ifdef DISPLAY_ENABLED
    SilabsLCD::Screen_e screen;
    GetLCD().GetScreen(screen);
    if (screen == SilabsLCD::Screen_e::DemoScreen)
    {
        BaseApplication::PostUpdateDisplayEvent(SilabsLCD::Screen_e::DemoScreen);
    }
#endif
}
