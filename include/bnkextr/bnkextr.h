#pragma once

#ifndef BNKEXTR_BNKEXTR_H
#define BNKEXTR_BNKEXTR_H

#include <iostream>
#include <string>
#include <array>
#include <variant>
#include <unordered_map>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/experimental/coro.hpp>

#include "bnkextr/structs.h"

namespace
{

using EventMap = std::unordered_map<uint32_t, bnkextr::EventObject>;
using EventActionMap = std::unordered_map<uint32_t, bnkextr::EventActionObject>;

template<size_t N>
bool compare(const std::array<uint8_t, N>& arr, const std::string& str)
{
    return std::equal(arr.cbegin(), arr.cend(), str.cbegin());
}

auto make_path(const std::string& filename)
{
    boost::filesystem::path path{filename};
    path = boost::filesystem::absolute(path);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
    return boost::filesystem::path{R"(\\?\)"}.concat(path);
#else
    return path;
#endif
}

} // namespace

namespace bnkextr
{

using ExtractResult = std::variant<WEMFile, Object>;

// TODO: needed?
template<typename T>
boost::asio::experimental::coro<void, T>
read_struct(boost::asio::stream_file& file)
{
    constexpr auto size = sizeof(T);
    T structure{};
    auto buf = reinterpret_cast<uint8_t*>(&structure);

    const auto num_read = co_await boost::asio::async_read(
        file,
        boost::asio::buffer(buf, size),
        boost::asio::deferred
    );

    co_return structure;
}

template<typename T>
boost::asio::experimental::coro<void, std::optional<T>>
maybe_read_struct(boost::asio::stream_file& file)
{
    constexpr auto size = sizeof(T);
    T structure{};
    auto buf = reinterpret_cast<uint8_t*>(&structure);

    // TODO: better error handling here?
    const auto [ec, num_read] = co_await boost::asio::async_read(
        file,
        boost::asio::buffer(buf, size),
        boost::asio::as_tuple(boost::asio::deferred)
    );

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

boost::asio::experimental::coro<void>
handle_object(
    boost::asio::stream_file& bank_file,
    const BankHeader& bank_header,
    const Object& object,
    EventMap& event_objects,
    EventActionMap& event_action_objects)
{
    EventObject event{};
    EventActionObject event_action{};
    std::optional<EventActionScope> scope_result;
    std::optional<EventActionType> type_result;
    std::optional<uint32_t> id_result;
    std::optional<uint8_t> count_result;
    std::optional<EventActionParameterType> param_type_result;
    std::optional<uint8_t> param_result;

    switch (object.type)
    {
        case ObjectType::Event:
            if (bank_header.version >= 134)
            {
                count_result = co_await maybe_read_struct<uint8_t>(bank_file);
                event.action_count = count_result.value();
                event.action_count = static_cast<uint32_t>(event.action_count);
            }
            else
            {
                count_result = co_await maybe_read_struct<uint32_t>(bank_file);
                event.action_count = count_result.value();
            }

            for (auto i = 0U; i < event.action_count; ++i)
            {
                const auto action_id_result = co_await maybe_read_struct<uint32_t>(bank_file);
                event.action_ids.emplace_back(action_id_result.value());
            }

            event_objects[object.id] = event;
            break;
        case ObjectType::EventAction:
            scope_result = co_await maybe_read_struct<EventActionScope>(bank_file);
            event_action.scope = scope_result.value_or(EventActionScope{});
            type_result = co_await maybe_read_struct<EventActionType>(bank_file);
            event_action.action_type = type_result.value_or(EventActionType{});
            id_result = co_await maybe_read_struct<uint32_t>(bank_file);
            event_action.game_object_id = id_result.value_or(0);

            bank_file.seek(1, boost::asio::file_base::seek_cur);

            count_result = co_await maybe_read_struct<uint8_t>(bank_file);
            event_action.parameter_count = count_result.value_or(0);

            for (auto j = 0U; static_cast<size_t>(event_action.parameter_count); ++j)
            {
                param_type_result = co_await maybe_read_struct<EventActionParameterType>(
                    bank_file);
                event_action.parameters_types.emplace_back(param_type_result.value_or(
                    EventActionParameterType{}));
            }

            for (auto j = 0U; static_cast<size_t>(event_action.parameter_count); ++j)
            {
                param_result = co_await maybe_read_struct<uint8_t>(bank_file);
                event_action.parameters.emplace_back(param_result.value_or(0));
            }

            bank_file.seek(1, boost::asio::file_base::seek_cur);
            bank_file.seek(object.size - 13 - event_action.parameter_count * 2,
                           boost::asio::file_base::seek_cur);

            event_action_objects[object.id] = event_action;

            break;
        default:
            // Just ignore for now. Not all are known.
            // throw std::runtime_error(
            //     "invalid object type " + std::to_string(
            //         static_cast
            //             <std::underlying_type<ObjectType>::type>(
            //             object.type))
            // );
            co_return;
    }
}

boost::asio::experimental::coro<ExtractResult, void>
extract_file(boost::asio::stream_file& bank_file,
             bool swap_byte_order = false)
{
    Index index{};
    std::vector<Index> wem_indices;
    BankHeader bank_header{};
    EventMap event_objects{};
    EventActionMap event_action_objects{};

    while (auto s_result = co_await maybe_read_struct<Section>(bank_file))
    {
        auto section = s_result.value();

        if (swap_byte_order)
        {
            section.size = std::byteswap(section.size);
        }

        // const auto section_pos = pos;
        const auto section_pos = bank_file.seek(0, boost::asio::file_base::seek_cur);

        const auto sign = section.sign;
        if (compare(sign, "BKHD"))
        {
            auto bh_result = co_await maybe_read_struct<BankHeader>(bank_file);
            bank_header = bh_result.value();
            bank_file.seek(section.size - sizeof(BankHeader),
                           boost::asio::file_base::seek_cur);
        }
        else if (compare(sign, "DIDX"))
        {
            // WEM file indices.
            for (auto i = 0U; i < section.size; i += sizeof(Index))
            {
                auto i_result = co_await maybe_read_struct<Index>(bank_file);
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

                std::vector<uint8_t> data(size, 0);
                co_await boost::asio::async_read(
                    bank_file,
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
            auto object_count_result = co_await maybe_read_struct<uint32_t>(bank_file);
            const auto object_count = object_count_result.value();

            for (auto i = 0U; i < object_count; ++i)
            {
                auto o_result = co_await maybe_read_struct<Object>(bank_file);
                // Ignore failed object reads.
                if (!o_result)
                {
                    continue;
                }

                auto object = o_result.value();

                // TODO: make a class to have these as members?
                co_await handle_object(
                    bank_file, bank_header, object, event_objects,
                    event_action_objects);

                bank_file.seek(object.size - sizeof(uint32_t),
                               boost::asio::file_base::seek_cur);

                co_yield object;
            }
        }

        bank_file.seek(
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
    const auto path = make_path(filename);

    std::cout << "extract_file_task\n";
    std::cout << "path     : " << path << "\n";
    std::cout << "filename : " << filename << "\n";

    boost::asio::stream_file bank_file{
        ctx,
        path.string(),
        boost::asio::stream_file::read_only
    };

    auto exc = extract_file(bank_file, swap_byte_order);
    while (auto result = co_await exc.async_resume(boost::asio::use_awaitable))
    {
        auto result_variant = result.value();
        if (std::holds_alternative<WEMFile>(result_variant))
        {
            auto wem_file = std::get<WEMFile>(result_variant);
            std::cout << "id       : " << wem_file.index.id << "\n";
            std::cout << "data len : " << wem_file.data.size() << "\n";
        }
        else if (std::holds_alternative<Object>(result_variant))
        {
            auto object = std::get<Object>(result_variant);
            std::cout << "id       : " << object.id << "\n";
            std::cout << "type     : " << static_cast<int>(object.type) << "\n";
            std::cout << "size     : " << object.size << "\n";
        }
    }

    co_return;
}

} // namespace bnkextr

#endif // BNKEXTR_BNKEXTR_H
