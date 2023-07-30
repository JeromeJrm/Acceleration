#include <SimpleIni.h>
#include <math.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <chrono>
#include <cmath>
#include <thread>
namespace logger = SKSE::log;

static float AccMaxSpeed = 100.0f;
static float AccMinSpeed = 1.0f;
static float fAccJumpBoost = 40.0f;
static bool bAccRotationDecrease = true;
static bool bAccDisableOnWeaponDraw = false;

static bool bAccDisable = false;
static float fAccRoationMult = 1.0f;
static float fAccSpeedupMult = 1.0f;
static float fAccStopSp = 0.85f;

static float AccMinSpeedS = 45.0f;
static float AccMinSpeedD = 65.0f;

static float AccSp = 1.6f;
static float AccSpS = 1.6f;
static float AccSpD = 3.8f;

static float AccHolder = 0.0f;

static bool bInSpeedCheck = false;

static bool bAccMenuIsOpen = false;

static bool bAccIsInAcc = false;
static bool bAccIsInDeAcc = false;

static bool bIsMoving = false;
static bool bIsInMoveCheck = false;
static int iIsMovingCount = 0;

static bool bIsWeapUtil = false;
static bool bIsInWeapUtilCheck = false;
static int iIsWeapUtil = 0;
static bool bAccIsSprinting = false;
static bool bAccIsWeapDrawn = false;

static bool bAccIsInShort = false;
static float fAccAngleDiffSmall = 0.0f;
const float PI = 3.14159265358979323846f;
static float fAccSubstSpeed = 0.0f;

static bool bAccIsInJumpBoost = false;
static bool bAccIsInPlayerStateCheck = false;

static bool bAccGameLoaded = false;
static bool bAccMoveStop = false;
static bool bAccCanDec = true;
static int iIniRefreshHK = 28;

void doLoadGame() {
    bAccGameLoaded = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bAccGameLoaded = false;
}

void getShortAngleDiff() {
    auto* playRf = RE::PlayerCharacter::GetSingleton();
    auto pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);
    bAccIsInShort = true;
    while (true) {
        auto currAngleZ = playRf->GetAngleZ();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));  // pause for 100 milliseconds
        auto newAngleZ = playRf->GetAngleZ();

        fAccAngleDiffSmall = currAngleZ - newAngleZ;
        while (fAccAngleDiffSmall < -PI) {
            fAccAngleDiffSmall += 2 * PI;
        }
        while (fAccAngleDiffSmall >= PI) {
            fAccAngleDiffSmall -= 2 * PI;
        }

        if (fAccAngleDiffSmall < 0) {
            fAccAngleDiffSmall = fAccAngleDiffSmall * -1.0f;
        }
        fAccSubstSpeed = fAccAngleDiffSmall / (2 * PI) * 60;

        float fAccSubstSpeedTwo = powf(fAccSubstSpeed, 2.0f);

        pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);
        if ((pSpeed - fAccSubstSpeed) > AccMinSpeed && bIsWeapUtil == false && bAccRotationDecrease == true &&
            bAccDisable == false) {
            playRf->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult,
                                                       pSpeed - (fAccSubstSpeedTwo * fAccRoationMult));
            pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);

        } else if ((pSpeed - (fAccSubstSpeedTwo * fAccRoationMult)) < AccMinSpeed && bIsWeapUtil == false &&
                   bAccRotationDecrease == true && bAccDisable == false) {
            playRf->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, AccMinSpeed);
            pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);
        }
        if (bAccGameLoaded == true) {
            bAccIsInShort = false;
            break;
        }
    }
}

void AccJumpBoost() {
    bAccIsInJumpBoost = true;
    auto* playRf = RE::PlayerCharacter::GetSingleton();
    if (playRf) {
        auto pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);

        pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);
        if (pSpeed + 30.0f < AccMaxSpeed + 31.0f)
            playRf->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, pSpeed + fAccJumpBoost);
        pSpeed = playRf->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (pSpeed >= AccMaxSpeed) {
            playRf->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, AccMaxSpeed);
        }
        bAccIsInJumpBoost = false;
    }
}

void AccGetIni() {
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(L"Data\\SKSE\\Plugins\\SkyrimMotionControl.ini");
    AccMaxSpeed = ini.GetDoubleValue("Global", "MaxSpeed", 100.0f);
    fAccJumpBoost = ini.GetDoubleValue("Global", "JumpBoost", 40.0f);
    if (fAccJumpBoost < 0.0f) {
        fAccJumpBoost = 0.0f;
    }
    bAccRotationDecrease = ini.GetBoolValue("Global", "RotationDecrease", true);
    fAccRoationMult = ini.GetBoolValue("Global", "RotationDecreaseMultiplier", 1.0f);
    if (fAccRoationMult < 0.01f) {
        fAccRoationMult = 0.01f;
    }
    bAccDisableOnWeaponDraw = ini.GetBoolValue("Global", "DisableOnWeaponDrawn", false);
    fAccSpeedupMult = ini.GetDoubleValue("Global", "SpeedupMultiplier", 1.0f);
    if (fAccSpeedupMult < 0.01f) {
        fAccSpeedupMult = 0.01f;
    }

    bAccMoveStop = ini.GetBoolValue("Global", "InstantMotionStop", false);

    iIniRefreshHK = ini.GetLongValue("Global", "RefreshHotkey", 28);
    if (iIniRefreshHK < 1 || iIniRefreshHK > 281) {
    
		iIniRefreshHK = 28;
	}
    logger::info("Refreshed INI");
}

void AccPlayerStateTests() {
    bAccIsInPlayerStateCheck = true;
    auto* AccPlayer = RE::PlayerCharacter::GetSingleton();

    while (true) {
        if (AccPlayer) {
            if (AccPlayer->AsActorState()->actorState1.sprinting) {
                bAccIsSprinting = true;

            } else {
                bAccIsSprinting = false;
            }

            if (AccPlayer->AsActorState()->actorState1.meleeAttackState != RE::ATTACK_STATE_ENUM::kNone ||
                AccPlayer->AsActorState()->actorState2.wantBlocking) {
                bIsWeapUtil = true;
                AccPlayer->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, AccMaxSpeed);
            }

            if (!AccPlayer->AsActorState()->actorState2.wantBlocking &&
                AccPlayer->AsActorState()->actorState1.meleeAttackState == RE::ATTACK_STATE_ENUM::kNone) {
                bIsWeapUtil = false;
            }
            
            if (AccPlayer->AsActorState()->IsWeaponDrawn()) {
                AccSp = AccSpD;
                bAccIsWeapDrawn = true;
                if (bAccDisableOnWeaponDraw == true) {
                    bAccDisable = true;
                    AccPlayer->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, AccMaxSpeed);
                }
                if (bAccDisableOnWeaponDraw == false) {
                    bAccDisable = false;
                }
            } else {
                AccSp = AccSpS;
                bAccIsWeapDrawn = false;
                bAccDisable = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (bAccGameLoaded == true) {
            bAccIsInPlayerStateCheck = false;
            break;
        }
    }
}

void MoveCheck() {
    bIsInMoveCheck = true;
    while (true) {
        int oldC = iIsMovingCount;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int nwC = iIsMovingCount;
        if (oldC < nwC) {
            bIsMoving = true;
            iIsMovingCount = 0;
        } else if (oldC == nwC) {
            bIsMoving = false;
            iIsMovingCount = 0;
        }

        if (bAccGameLoaded == true) {
            bIsInMoveCheck = false;
            break;
        }
    }
}



void AccPlayerAcceleration() {
    if (bAccIsInAcc == false) {
        bAccIsInAcc = true;
        auto* AccPlayer = RE::PlayerCharacter::GetSingleton();

        if (AccPlayer != nullptr) {
            auto AccPlayerSpeed = AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);

            AccHolder = AccSp * (((AccMaxSpeed - AccPlayerSpeed) / 100.0f) + (AccMinSpeed / 100.0f));

            float AccHolderTwo = powf(AccHolder, 2.5f);

            if (AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) < AccMaxSpeed) {
                if (AccHolderTwo < 0.5f) {
                    AccHolderTwo = 0.5f;
                }
                if (AccHolderTwo > AccSpD) {
                    AccHolderTwo = AccSpD;
                }
                AccPlayer->AsActorValueOwner()->ModActorValue(RE::ActorValue::kSpeedMult,
                                                              AccHolderTwo * fAccSpeedupMult);
                AccPlayerSpeed = AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        bAccIsInAcc = false;
    }
}

void AccPlayerDeacceleration() {
    bAccIsInDeAcc = true;
    bAccIsInAcc = false;
    while (true) {
        if (bAccIsInAcc == false && bIsMoving == false) {
            auto* AccPlayer = RE::PlayerCharacter::GetSingleton();
            if (AccPlayer != nullptr) {
                if (bAccMoveStop == false) {
                
                if (bAccIsInAcc == false && bIsMoving == false && bIsWeapUtil == false && bAccIsSprinting == false &&
                        bAccDisable == false) {
                    bool madeNegative = false;
                    if (AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) >= AccMinSpeed) {
                        if (fAccStopSp < 1.0f) {
                            AccPlayer->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult,
                                                                          -AccMaxSpeed * fAccStopSp);
                        }
                        if (fAccStopSp == 1.0f) {
                            AccPlayer->AsActorValueOwner()->SetActorValue(
                                RE::ActorValue::kSpeedMult,
                                AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) * (-20.00f));
                        }

                        madeNegative = true;
                        bAccCanDec = false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                    if (AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) < AccMinSpeed &&
                        madeNegative == true) {
                        AccPlayer->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, AccMinSpeed);
                    }
                }
                }
                if (bAccMoveStop == true) {
                if (bAccIsInAcc == false && bIsMoving == false && bIsWeapUtil == false && bAccIsSprinting == false &&
                        bAccDisable == false && !AccPlayer->AsActorState()->actorState1.movingBack &&
                        !AccPlayer->AsActorState()->actorState1.movingForward &&
                    !AccPlayer->AsActorState()->actorState1.movingLeft &&
                    !AccPlayer->AsActorState()->actorState1.movingRight) {
                    bool madeNegative = false;
                    if (AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) >= AccMinSpeed) {
                        if (fAccStopSp < 1.0f) {
                            AccPlayer->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult,
                                                                          -AccMaxSpeed * fAccStopSp);
                        }
                        if (fAccStopSp == 1.0f) {
                            AccPlayer->AsActorValueOwner()->SetActorValue(
                                RE::ActorValue::kSpeedMult,
                                AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) * (-20.00f));
                        }

                        madeNegative = true;
                        bAccCanDec = false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                    if (AccPlayer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult) < AccMinSpeed &&
                        madeNegative == true) {
                        AccPlayer->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, AccMinSpeed);
                    }
                }
                }
            }
        }

        if (bAccGameLoaded == true) {
            bAccIsInDeAcc = false;
            break;
        }
    }
}

class AccEventSink : public RE::BSTEventSink<RE::InputEvent*> {
    AccEventSink() = default;
    AccEventSink(const AccEventSink&) = delete;
    AccEventSink(AccEventSink&&) = delete;
    AccEventSink& operator=(const AccEventSink&) = delete;
    AccEventSink& operator=(AccEventSink&&) = delete;

public:
    static AccEventSink* GetSingleton() {
        static AccEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!eventPtr) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* event = *eventPtr;

        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        

        if (event) { 
            if (event->QUserEvent() == "Forward" || event->QUserEvent() == "Move" ||
                event->QUserEvent() == "Strafe Left" || event->QUserEvent() == "Strafe Right" ||
                event->QUserEvent() == "Back" || event->QUserEvent() == "Sprint" || event->QUserEvent() == "Jump" ||
                event->QUserEvent() == "Sneak") {
                iIsMovingCount = iIsMovingCount + 1;

                std::thread AccPlayerAccelerationThread(AccPlayerAcceleration);
                AccPlayerAccelerationThread.detach();
                
                if (bIsInMoveCheck == false) {
                    std::thread AccPlayerMoveCheckThread(MoveCheck);
                    AccPlayerMoveCheckThread.detach();
                }
                if (bAccIsInDeAcc == false) {
                    std::thread AccPlayerDeaccelerationThread(AccPlayerDeacceleration);
                    AccPlayerDeaccelerationThread.detach();
                }

                if (bAccIsInShort == false) {
                    std::thread shortangle(getShortAngleDiff);
                    shortangle.detach();
                }

                if (bAccIsInPlayerStateCheck == false) {
                    std::thread AccPlayerStateCheckThread(AccPlayerStateTests);
                    AccPlayerStateCheckThread.detach();
                }
            }

            if (event->QUserEvent() == "Jump") {
                if (fAccJumpBoost > 0.0f) {
                    if (bAccIsInJumpBoost == false) {
                        std::thread AccPlayerJumpThread(AccJumpBoost);
                        AccPlayerJumpThread.detach();
                    }
                }
                
            }
            if (event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
                auto* buttonEvent = event->AsButtonEvent();
                auto dxScanCode = buttonEvent->GetIDCode();
                if (dxScanCode == iIniRefreshHK) {
                    AccGetIni();
                    
                }
                
            
            }

        }

        return RE::BSEventNotifyControl::kContinue;
    }
};

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kInputLoaded) {
        RE::BSInputDeviceManager::GetSingleton()->AddEventSink(AccEventSink::GetSingleton());
        AccGetIni();
    }

    if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        
        std::thread AccGameLoaded(doLoadGame);
        AccGameLoaded.detach();
    }
}

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();
    
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    return true;
}