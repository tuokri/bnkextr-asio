#pragma once

#ifndef BNKEXTR_ASIO_STRUCTS_H
#define BNKEXTR_ASIO_STRUCTS_H

#include <vector>
#include <cstdint>
#include <array>

namespace bnkextr
{

#pragma pack(push, 1)
struct Index
{
    uint32_t id;
    uint32_t offset;
    uint32_t size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Section
{
    std::array<uint8_t, 4> sign;
    uint32_t size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BankHeader
{
    uint32_t version;
    uint32_t id;
};
#pragma pack(pop)

enum class ObjectType: uint8_t
{
    SoundEffectOrVoice = 2,
    EventAction = 3,
    Event = 4,
    RandomOrSequenceContainer = 5,
    SwitchContainer = 6,
    ActorMixer = 7,
    AudioBus = 8,
    BlendContainer = 9,
    MusicSegment = 10,
    MusicTrack = 11,
    MusicSwitchContainer = 12,
    MusicPlaylistContainer = 13,
    Attenuation = 14,
    DialogueEvent = 15,
    MotionBus = 16,
    MotionFx = 17,
    Effect = 18,
    Unknown = 19,
    AuxiliaryBus = 20
};

#pragma pack(push, 1)
struct Object
{
    ObjectType type;
    uint32_t size;
    uint32_t id;
};
#pragma pack(pop)

struct EventObject
{
    uint32_t action_count;
    std::vector<uint32_t> action_ids;
};

enum class EventActionScope: uint8_t
{
    SwitchOrTrigger = 1,
    Global = 2,
    GameObject = 3,
    State = 4,
    All = 5,
    AllExcept = 6
};

enum class EventActionType: uint8_t
{
    Stop = 1,
    Pause = 2,
    Resume = 3,
    Play = 4,
    Trigger = 5,
    Mute = 6,
    UnMute = 7,
    SetVoicePitch = 8,
    ResetVoicePitch = 9,
    SetVoiceVolume = 10,
    ResetVoiceVolume = 11,
    SetBusVolume = 12,
    ResetBusVolume = 13,
    SetVoiceLowPassFilter = 14,
    ResetVoiceLowPassFilter = 15,
    EnableState = 16,
    DisableState = 17,
    SetState = 18,
    SetGameParameter = 19,
    ResetGameParameter = 20,
    SetSwitch = 21,
    ToggleBypass = 22,
    ResetBypassEffect = 23,
    Break = 24,
    Seek = 25
};

enum class EventActionParameterType: uint8_t
{
    Delay = 0x0E,
    Play = 0x0F,
    Probability = 0x10
};

struct EventActionObject
{
    EventActionScope scope;
    EventActionType action_type;
    uint32_t game_object_id;
    uint8_t parameter_count;
    std::vector<EventActionParameterType> parameters_types;
    std::vector<uint8_t> parameters;
};

struct WEMFile
{
    Index index;
    std::vector<uint8_t> data;
};

} // bnkextr

#endif // BNKEXTR_ASIO_STRUCTS_H
