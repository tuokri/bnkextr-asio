#pragma once

#ifndef BNKEXTR_BNKEXTR_H
#define BNKEXTR_BNKEXTR_H

#include <iostream>
#include <string>
#include <array>
#include <variant>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/experimental/coro.hpp>

namespace
{

template<size_t N>
bool compare(const std::array<char, N>& arr, const std::string& str)
{
    return std::equal(arr.cbegin(), arr.cend(), str.cbegin());
}

} // namespace

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
    std::array<char, 4> sign;
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

struct WEMFile
{
    Index index;
    std::vector<char> data;
};

using ExtractResult = std::variant<WEMFile, Object>;

template<typename T>
boost::asio::experimental::coro<void, std::optional<T>>
read_struct(boost::asio::stream_file& file,
            size_t& pos)
{
    constexpr auto size = sizeof(T);
    T structure{};
    auto buf = reinterpret_cast<char*>(&structure);

    // TODO: better error handling here?
    const auto [ec, num_read] = co_await boost::asio::async_read(
        file,
        boost::asio::buffer(buf, size),
        boost::asio::as_tuple(boost::asio::deferred)
    );

    pos += num_read;

    if (ec == boost::asio::error::eof)
    {
        co_return std::nullopt;
    }
    else if (ec)
    {
        throw boost::system::system_error(ec);
    }

    co_return structure;
}

// so we want a function that given some input (stream, filename?)
// -> asynchronously process said input
// -> yield wem files (also need data_offset)
// -> yield objects

boost::asio::experimental::coro<ExtractResult, void>
extract_file(boost::asio::stream_file& bnk_file,
             bool swap_byte_order = false)
{
    std::cout << "extract_file" << std::endl;

    // open stream
    // yield objects
    // read and store until data_offset found (pseudo blocking)
    // yield wem files (use data_offset here)

    // yield type should be some wrapper type or union?

    Index index{};
    size_t pos = 0;
    std::vector<Index> wem_indices;

    while (auto s_result = co_await read_struct<Section>(bnk_file, pos))
    {
        auto section = s_result.value();

        if (swap_byte_order)
        {
            section.size = std::byteswap(section.size);
        }

        const auto section_pos = pos;

        const auto sign = section.sign;
        if (compare(sign, "BKHD"))
        {
            auto bh_result = co_await read_struct<BankHeader>(bnk_file, pos);
            auto bank_header = bh_result.value();
            pos = bnk_file.seek(section.size - sizeof(BankHeader),
                                boost::asio::file_base::seek_cur);
        }
        else if (compare(sign, "DIDX"))
        {
            // WEM file indices.
            for (auto i = 0U; i < section.size; i += sizeof(Index))
            {
                auto i_result = co_await read_struct<Index>(bnk_file, pos);
                index = i_result.value();
                wem_indices.emplace_back(index);
            }
        }
        else if (compare(sign, "STID"))
        {
            // Not implemented.
        }
        else if (compare(sign, "DATA"))
        {
            for (auto& [id, offset, size]: wem_indices)
            {
                if (swap_byte_order)
                {
                    size = std::byteswap(size);
                    offset = std::byteswap(offset);
                }

                std::vector<char> data(size, 0);
                co_await boost::asio::async_read(
                    bnk_file,
                    boost::asio::buffer(data, size),
                    boost::asio::deferred
                );
                WEMFile wem_file{
                    .index{id, offset, size},
                    .data{data},
                };
                co_yield wem_file;
            }
        }
        else if (compare(sign, "HIRC"))
        {
            auto object_count = co_await read_struct<uint32_t>(bnk_file, pos);

            for (auto i = 0U; i < object_count; ++i)
            {
                auto o_result = co_await read_struct<Object>(bnk_file, pos);
                auto object = o_result.value();

                // TODO: check object type.

                bnk_file.seek(object.size - sizeof(uint32_t),
                              boost::asio::file_base::seek_cur);
            }
        }

        pos = bnk_file.seek(
            section_pos + section.size,
            boost::asio::file_base::seek_set);
    }

    co_return;
}

boost::asio::awaitable<void>
extract_file_task(boost::asio::io_context& ctx,
                  const std::string& filename,
                  bool swap_byte_order = false)
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

    std::vector<uint32_t> ids;

    auto exc = extract_file(bnk_file, swap_byte_order);
    while (auto result = co_await exc.async_resume(boost::asio::use_awaitable))
    {
        auto result_variant = result.value();
        if (std::holds_alternative<WEMFile>(result_variant))
        {
            auto wem_file = std::get<WEMFile>(result_variant);
            std::cout << "id       : " << wem_file.index.id << "\n";
            std::cout << "data len : " << wem_file.data.size() << "\n";

            ids.emplace_back(wem_file.index.id);
        }
    }

    std::cout << "ids.size(): " << ids.size() << "\n";

    std::cout << "?\n" << std::endl;

    co_return;
}

} // namespace bnkextr

#endif //BNKEXTR_BNKEXTR_H
