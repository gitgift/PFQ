#include <cstdio>
#include <string>
#include <stdexcept>

#include <pfq.hpp>
using namespace net;

int
main(int argc, char *argv[])
{
    if (argc < 2)
        throw std::runtime_error(std::string("usage: ").append(argv[0]).append(" dev"));

    pfq r(128);

    r.bind(argv[1], pfq::any_queue);

    r.timestamp_enable(true);

    r.set_group_function(r.group_id(), "steer-dummy");

    r.enable();

    for(int i = 0; i < 5;++i)
    {
            r.read( 1000000 /* timeout: micro */);
    }

    int context = 42;
    r.set_group_function_context(r.group_id(), context);

    for(int i = 0; i < 5;++i)
    {
            r.read( 1000000 /* timeout: micro */);
    }

    r.reset_group(r.group_id());

    for(int i = 0; i < 5;++i)
    {
            r.read( 1000000 /* timeout: micro */);
    }

    return 0;
}

