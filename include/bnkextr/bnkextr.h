#pragma once

#ifndef BNKEXTR_BNKEXTR_H
#define BNKEXTR_BNKEXTR_H

#include <iostream>
#include <string>
#include <array>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/experimental/coro.hpp>

namespace bnkextr
{

#pragma pack(push, 1)
struct Index
{
    std::uint32_t id;
    std::uint32_t offset;
    std::uint32_t size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Section
{
    char sign[4];
    std::uint32_t size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BankHeader
{
    std::uint32_t version;
    std::uint32_t id;
};
#pragma pack(pop)

enum class ObjectType: std::int8_t
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
    std::uint32_t size;
    std::uint32_t id;
};
#pragma pack(pop)

struct EventObject
{
    std::uint32_t action_count;
    std::vector<std::uint32_t> action_ids;
};

enum class EventActionScope: std::int8_t
{
    SwitchOrTrigger = 1,
    Global = 2,
    GameObject = 3,
    State = 4,
    All = 5,
    AllExcept = 6
};

enum class EventActionType: std::int8_t
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

enum class EventActionParameterType: std::int8_t
{
    Delay = 0x0E,
    Play = 0x0F,
    Probability = 0x10
};

struct EventActionObject
{
    EventActionScope scope;
    EventActionType action_type;
    std::uint32_t game_object_id;
    std::uint8_t parameter_count;
    std::vector<EventActionParameterType> parameters_types;
    std::vector<std::int8_t> parameters;
};

template<typename T>
boost::asio::experimental::coro<void, bool>
read_structure(boost::asio::stream_file& file,
               T& structure)
{
    constexpr auto size = sizeof(T);
//    std::cout << "size: " << size << "\n";
    auto buf = reinterpret_cast<char*>(&structure);

    auto [ec, n] = co_await boost::asio::async_read(
        file,
        boost::asio::buffer(buf, size),
        // TODO: upgrade boost and use deferred here?
        boost::asio::as_tuple(boost::asio::experimental::use_coro)
    );

    std::cout << "buf: " << std::string{buf, size} << "\n";
    std::cout << "ec : " << ec.message() << "\n";
    std::cout << "n  : " << n << "\n";

    const auto success = !ec;
    co_return success;
};

// so we want a function that given some input (stream, filename?)
// -> asynchronously process said input
// -> yield wem files (also need data_offset)
// -> yield objects

boost::asio::experimental::coro<void, void>
extract_file(boost::asio::stream_file& bnk_file,
             bool swap_byte_order = false)
{
    std::cout << "extract_file" << std::endl;

    // open stream
    // yield objects
    // read and store until data_offset found (pseudo blocking)
    // yield wem files (use data_offset here)

    // yield type should be some wrapper type or union?

    Section content_section{};
    size_t data_offset = 0;
    uint64_t total = 0;

    while (bnk_file.is_open())
    {
        // auto rs = read_structure(bnk_file, content_section);
        const auto success = co_await read_structure(bnk_file, content_section);
        total += content_section.size;
        std::cout << "total: " << total << "\n";

        if (!success)
        {
            break;
        }

        bnk_file.seek(
            content_section.size,
            boost::asio::file_base::seek_cur);

        // std::cout << "success: " << std::to_string(success) << "\n";
    }

    co_return;
}

boost::asio::awaitable<void>
extract_file_task(boost::asio::io_context& ctx,
                  const std::string& filename)
{
    boost::filesystem::path path = boost::filesystem::current_path();
    std::cout << "extract_file_task\n";
    std::cout << "path     : " << path << "\n";
    std::cout << "filename : " << filename << "\n";

    boost::asio::stream_file bnk_file{
        ctx,
        filename,
        boost::asio::stream_file::read_only
    };

    std::cout << "file init\n";

    auto exc = extract_file(bnk_file);
    co_await exc.async_resume(boost::asio::use_awaitable);

    std::cout << "?\n" << std::endl;

    co_return;
}

} // namespace bnkextr

#endif //BNKEXTR_BNKEXTR_H
