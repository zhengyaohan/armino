/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include "AppConfig.h"
#include "AppEvent.h"
#include "LEDWidget.h"
#include "LightingManager.h"
#include "ThreadUtil.h"

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/ErrorStr.h>
#include <platform/CHIPDeviceLayer.h>
#include <system/SystemClock.h>

#if CONFIG_CHIP_OTA_REQUESTOR
#include <app/clusters/ota-requestor/BDXDownloader.h>
#include <app/clusters/ota-requestor/OTARequestor.h>
#include <platform/GenericOTARequestorDriver.h>
#include <platform/nrfconnect/OTAImageProcessorImpl.h>
#endif

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

LOG_MODULE_DECLARE(app);

using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace {

constexpr int kFactoryResetTriggerTimeout      = 3000;
constexpr int kFactoryResetCancelWindowTimeout = 3000;
constexpr int kExtDiscoveryTimeoutSecs         = 20;
constexpr int kAppEventQueueSize               = 10;
constexpr uint8_t kButtonPushEvent             = 1;
constexpr uint8_t kButtonReleaseEvent          = 0;
constexpr chip::EndpointId kIdentifyEndpointId = 1;
constexpr uint32_t kIdentifyBlinkRateMs        = 500;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
k_timer sFunctionTimer;

Identify sIdentify = { kIdentifyEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
                       EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LED };

LEDWidget sStatusLED;
LEDWidget sIdentifyLED;
LEDWidget sUnusedLED;

bool sIsThreadProvisioned = false;
bool sIsThreadEnabled     = false;
bool sHaveBLEConnections  = false;

#if CONFIG_CHIP_OTA_REQUESTOR
GenericOTARequestorDriver sOTARequestorDriver;
OTAImageProcessorImpl sOTAImageProcessor;
chip::BDXDownloader sBDXDownloader;
chip::OTARequestor sOTARequestor;
#endif

} // namespace

AppTask AppTask::sAppTask;

int AppTask::Init()
{
    // Initialize LEDs
    LEDWidget::InitGpio();
    LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

    sStatusLED.Init(SYSTEM_STATE_LED);
    sIdentifyLED.Init(DK_LED3);
    sUnusedLED.Init(DK_LED4);

    UpdateStatusLED();

    // Initialize buttons
    int ret = dk_buttons_init(ButtonEventHandler);
    if (ret)
    {
        LOG_ERR("dk_buttons_init() failed");
        return ret;
    }

    // Initialize timer user data
    k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
    k_timer_user_data_set(&sFunctionTimer, this);

#ifdef CONFIG_MCUMGR_SMP_BT
    GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
    GetDFUOverSMP().ConfirmNewImage();
#endif

    ret = LightingMgr().Init(LIGHTING_PWM_DEVICE, LIGHTING_PWM_CHANNEL);
    if (ret != 0)
        return ret;

    LightingMgr().SetCallbacks(ActionInitiated, ActionCompleted);

    // Init ZCL Data Model and start server
    chip::Server::GetInstance().Init();

    // Initialize device attestation config
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
    ConfigurationMgr().LogDeviceConfig();
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

#if defined(CHIP_DEVICE_CONFIG_ENABLE_EXTENDED_DISCOVERY)
    chip::app::DnssdServer::Instance().SetExtendedDiscoveryTimeoutSecs(kExtDiscoveryTimeoutSecs);
#endif

#if defined(CONFIG_CHIP_NFC_COMMISSIONING)
    PlatformMgr().AddEventHandler(ChipEventHandler, 0);
#endif

    InitOTARequestor();

    return 0;
}

void AppTask::InitOTARequestor()
{
#if CONFIG_CHIP_OTA_REQUESTOR
    sOTAImageProcessor.SetOTADownloader(&sBDXDownloader);
    sBDXDownloader.SetImageProcessorDelegate(&sOTAImageProcessor);
    sOTARequestorDriver.Init(&sOTARequestor, &sOTAImageProcessor);
    sOTARequestor.Init(&chip::Server::GetInstance(), &sOTARequestorDriver, &sBDXDownloader);
    chip::SetRequestorInstance(&sOTARequestor);
#endif
}

int AppTask::StartApp()
{
    int ret = Init();

    if (ret)
    {
        LOG_ERR("AppTask.Init() failed");
        return ret;
    }

    AppEvent event = {};

    while (true)
    {
        k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
        DispatchEvent(&event);
    }
}

void AppTask::LightingActionEventHandler(AppEvent * aEvent)
{
    LightingManager::Action_t action = LightingManager::INVALID_ACTION;
    int32_t actor                    = 0;

    if (aEvent->Type == AppEvent::kEventType_Lighting)
    {
        action = static_cast<LightingManager::Action_t>(aEvent->LightingEvent.Action);
        actor  = aEvent->LightingEvent.Actor;
    }
    else if (aEvent->Type == AppEvent::kEventType_Button)
    {
        action = LightingMgr().IsTurnedOn() ? LightingManager::OFF_ACTION : LightingManager::ON_ACTION;
        actor  = AppEvent::kEventType_Button;
    }

    if (action != LightingManager::INVALID_ACTION && !LightingMgr().InitiateAction(action, actor, 0, NULL))
        LOG_INF("Action is already in progress or active.");
}

void AppTask::ButtonEventHandler(uint32_t button_state, uint32_t has_changed)
{
    AppEvent button_event;
    button_event.Type = AppEvent::kEventType_Button;

    if (LIGHTING_BUTTON_MASK & button_state & has_changed)
    {
        button_event.ButtonEvent.PinNo  = LIGHTING_BUTTON;
        button_event.ButtonEvent.Action = kButtonPushEvent;
        button_event.Handler            = LightingActionEventHandler;
        sAppTask.PostEvent(&button_event);
    }

    if (FUNCTION_BUTTON_MASK & has_changed)
    {
        button_event.ButtonEvent.PinNo  = FUNCTION_BUTTON;
        button_event.ButtonEvent.Action = (FUNCTION_BUTTON_MASK & button_state) ? kButtonPushEvent : kButtonReleaseEvent;
        button_event.Handler            = FunctionHandler;
        sAppTask.PostEvent(&button_event);
    }

    if (THREAD_START_BUTTON_MASK & button_state & has_changed)
    {
        button_event.ButtonEvent.PinNo  = THREAD_START_BUTTON;
        button_event.ButtonEvent.Action = kButtonPushEvent;
        button_event.Handler            = StartThreadHandler;
        sAppTask.PostEvent(&button_event);
    }

    if (BLE_ADVERTISEMENT_START_BUTTON_MASK & button_state & has_changed)
    {
        button_event.ButtonEvent.PinNo  = BLE_ADVERTISEMENT_START_BUTTON;
        button_event.ButtonEvent.Action = kButtonPushEvent;
        button_event.Handler            = StartBLEAdvertisementHandler;
        sAppTask.PostEvent(&button_event);
    }
}

void AppTask::TimerEventHandler(k_timer * timer)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Timer;
    event.TimerEvent.Context = k_timer_user_data_get(timer);
    event.Handler            = FunctionTimerEventHandler;
    sAppTask.PostEvent(&event);
}

void AppTask::IdentifyStartHandler(Identify *)
{
    AppEvent event;
    event.Type    = AppEvent::kEventType_IdentifyStart;
    event.Handler = [](AppEvent *) { sIdentifyLED.Blink(kIdentifyBlinkRateMs); };
    sAppTask.PostEvent(&event);
}

void AppTask::IdentifyStopHandler(Identify *)
{
    AppEvent event;
    event.Type    = AppEvent::kEventType_IdentifyStop;
    event.Handler = [](AppEvent *) { sIdentifyLED.Set(false); };
    sAppTask.PostEvent(&event);
}

void AppTask::FunctionTimerEventHandler(AppEvent * aEvent)
{
    if (aEvent->Type != AppEvent::kEventType_Timer)
        return;

    // If we reached here, the button was held past kFactoryResetTriggerTimeout, initiate factory reset
    if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_SoftwareUpdate)
    {
        LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.", kFactoryResetTriggerTimeout);

        // Start timer for kFactoryResetCancelWindowTimeout to allow user to cancel, if required.
        sAppTask.StartTimer(kFactoryResetCancelWindowTimeout);
        sAppTask.mFunction = kFunction_FactoryReset;

        // Turn off all LEDs before starting blink to make sure blink is co-ordinated.
        sStatusLED.Set(false);
        sIdentifyLED.Set(false);
        sUnusedLED.Set(false);

        sStatusLED.Blink(500);
        sIdentifyLED.Blink(500);
        sUnusedLED.Blink(500);
    }
    else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
    {
        // Actually trigger Factory Reset
        sAppTask.mFunction = kFunction_NoneSelected;
        ConfigurationMgr().InitiateFactoryReset();
    }
}

#ifdef CONFIG_MCUMGR_SMP_BT
void AppTask::RequestSMPAdvertisingStart(void)
{
    AppEvent event;
    event.Type    = AppEvent::kEventType_StartSMPAdvertising;
    event.Handler = [](AppEvent *) { GetDFUOverSMP().StartBLEAdvertising(); };
    sAppTask.PostEvent(&event);
}
#endif

void AppTask::FunctionHandler(AppEvent * aEvent)
{
    if (aEvent->ButtonEvent.PinNo != FUNCTION_BUTTON)
        return;

    // To trigger software update: press the FUNCTION_BUTTON button briefly (< kFactoryResetTriggerTimeout)
    // To initiate factory reset: press the FUNCTION_BUTTON for kFactoryResetTriggerTimeout + kFactoryResetCancelWindowTimeout
    // All LEDs start blinking after kFactoryResetTriggerTimeout to signal factory reset has been initiated.
    // To cancel factory reset: release the FUNCTION_BUTTON once all LEDs start blinking within the
    // kFactoryResetCancelWindowTimeout
    if (aEvent->ButtonEvent.Action == kButtonPushEvent)
    {
        if (!sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_NoneSelected)
        {
            sAppTask.StartTimer(kFactoryResetTriggerTimeout);

            sAppTask.mFunction = kFunction_SoftwareUpdate;
        }
    }
    else
    {
        // If the button was released before factory reset got initiated, trigger a software update.
        if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_SoftwareUpdate)
        {
            sAppTask.CancelTimer();
            sAppTask.mFunction = kFunction_NoneSelected;

#ifdef CONFIG_MCUMGR_SMP_BT
            GetDFUOverSMP().StartServer();
#else
            LOG_INF("Software update is disabled");
#endif
        }
        else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
        {
            sIdentifyLED.Set(false);
            sUnusedLED.Set(false);
            UpdateStatusLED();
            sAppTask.CancelTimer();
            sAppTask.mFunction = kFunction_NoneSelected;
            LOG_INF("Factory Reset has been Canceled");
        }
    }
}

void AppTask::StartThreadHandler(AppEvent * aEvent)
{
    if (aEvent->ButtonEvent.PinNo != THREAD_START_BUTTON)
        return;

    if (chip::Server::GetInstance().AddTestCommissioning() != CHIP_NO_ERROR)
    {
        LOG_ERR("Failed to add test pairing");
    }

    if (!chip::DeviceLayer::ConnectivityMgr().IsThreadProvisioned())
    {
        StartDefaultThreadNetwork();
        LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration.");
    }
    else
    {
        LOG_INF("Device is commissioned to a Thread network.");
    }
}

void AppTask::StartBLEAdvertisementHandler(AppEvent * aEvent)
{
    if (aEvent->ButtonEvent.PinNo != BLE_ADVERTISEMENT_START_BUTTON)
        return;

    // Don't allow on starting Matter service BLE advertising after Thread provisioning.
    if (ConnectivityMgr().IsThreadProvisioned())
    {
        LOG_INF("NFC Tag emulation and Matter service BLE advertising not started - device is commissioned to a Thread network.");
        return;
    }

    if (ConnectivityMgr().IsBLEAdvertisingEnabled())
    {
        LOG_INF("BLE advertising is already enabled");
        return;
    }

    if (chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() != CHIP_NO_ERROR)
    {
        LOG_ERR("OpenBasicCommissioningWindow() failed");
    }
}

void AppTask::UpdateLedStateEventHandler(AppEvent * aEvent)
{
    if (aEvent->Type == AppEvent::kEventType_UpdateLedState)
    {
        aEvent->UpdateLedStateEvent.LedWidget->UpdateState();
    }
}

void AppTask::LEDStateUpdateHandler(LEDWidget & ledWidget)
{
    AppEvent event;
    event.Type                          = AppEvent::kEventType_UpdateLedState;
    event.Handler                       = UpdateLedStateEventHandler;
    event.UpdateLedStateEvent.LedWidget = &ledWidget;
    sAppTask.PostEvent(&event);
}

void AppTask::UpdateStatusLED()
{
    /* Update the status LED.
     *
     * If thread and service provisioned, keep the LED On constantly.
     *
     * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
     * rate of 100ms.
     *
     * Otherwise, blink the LED On for a very short time. */
    if (sIsThreadProvisioned && sIsThreadEnabled)
    {
        sStatusLED.Set(true);
    }
    else if (sHaveBLEConnections)
    {
        sStatusLED.Blink(100, 100);
    }
    else
    {
        sStatusLED.Blink(50, 950);
    }
}

void AppTask::ChipEventHandler(const ChipDeviceEvent * event, intptr_t /* arg */)
{
    switch (event->Type)
    {
    case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
        if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started)
        {
            if (NFCMgr().IsTagEmulationStarted())
            {
                LOG_INF("NFC Tag emulation is already started");
            }
            else
            {
                ShareQRCodeOverNFC(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
            }
        }
        else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped)
        {
            NFCMgr().StopTagEmulation();
        }
#endif
        sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
        UpdateStatusLED();
        break;
    case DeviceEventType::kThreadStateChange:
        sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
        sIsThreadEnabled     = ConnectivityMgr().IsThreadEnabled();
        UpdateStatusLED();
        break;
    default:
        break;
    }
}

void AppTask::CancelTimer()
{
    k_timer_stop(&sFunctionTimer);
    mFunctionTimerActive = false;
}

void AppTask::StartTimer(uint32_t aTimeoutInMs)
{
    k_timer_start(&sFunctionTimer, K_MSEC(aTimeoutInMs), K_NO_WAIT);
    mFunctionTimerActive = true;
}

void AppTask::ActionInitiated(LightingManager::Action_t aAction, int32_t aActor)
{
    if (aAction == LightingManager::ON_ACTION)
    {
        LOG_INF("Turn On Action has been initiated");
    }
    else if (aAction == LightingManager::OFF_ACTION)
    {
        LOG_INF("Turn Off Action has been initiated");
    }
    else if (aAction == LightingManager::LEVEL_ACTION)
    {
        LOG_INF("Level Action has been initiated");
    }
}

void AppTask::ActionCompleted(LightingManager::Action_t aAction, int32_t aActor)
{
    if (aAction == LightingManager::ON_ACTION)
    {
        LOG_INF("Turn On Action has been completed");
    }
    else if (aAction == LightingManager::OFF_ACTION)
    {
        LOG_INF("Turn Off Action has been completed");
    }
    else if (aAction == LightingManager::LEVEL_ACTION)
    {
        LOG_INF("Level Action has been completed");
    }

    if (aActor == AppEvent::kEventType_Button)
    {
        sAppTask.UpdateClusterState();
    }
}

void AppTask::PostLightingActionRequest(LightingManager::Action_t aAction)
{
    AppEvent event;
    event.Type                 = AppEvent::kEventType_Lighting;
    event.LightingEvent.Action = aAction;
    event.Handler              = LightingActionEventHandler;
    PostEvent(&event);
}

void AppTask::PostEvent(AppEvent * aEvent)
{
    if (k_msgq_put(&sAppEventQueue, aEvent, K_NO_WAIT) != 0)
    {
        LOG_INF("Failed to post event to app task event queue");
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    if (aEvent->Handler)
    {
        aEvent->Handler(aEvent);
    }
    else
    {
        LOG_INF("Event received with no handler. Dropping event.");
    }
}

void AppTask::UpdateClusterState()
{
    uint8_t onoff = LightingMgr().IsTurnedOn();

    // write the new on/off value
    EmberAfStatus status = emberAfWriteAttribute(1, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER, &onoff,
                                                 ZCL_BOOLEAN_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        LOG_ERR("Updating on/off cluster failed: %x", status);
    }

    uint8_t level = LightingMgr().GetLevel();

    status = emberAfWriteAttribute(1, ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID, CLUSTER_MASK_SERVER, &level,
                                   ZCL_INT8U_ATTRIBUTE_TYPE);

    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        LOG_ERR("Updating level cluster failed: %x", status);
    }
}