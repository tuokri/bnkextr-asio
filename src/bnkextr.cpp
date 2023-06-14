#include "bnkextr/bnkextr.h"

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>

namespace bnkextr
{

} // namespace bnkextr

constexpr auto bnk = "WwiseDefaultBank_WW_VOX_OBJ_ZedLanding.bnk";

int main()
{
    try
    {
        boost::asio::io_context ctx;

        const std::string test{bnk};

        co_spawn(
            ctx,
            bnkextr::extract_file_task(ctx, test),
            [](const std::exception_ptr& e)
            {
                if (e)
                {
                    std::rethrow_exception(e);
                }
            }
        );

        std::cout << "gonna run\n";
        ctx.run();
        std::cout << "ran\n";
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cout << "unhandled error\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
