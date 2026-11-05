#include "echoServer.hpp"
#include <csignal>
#include "runtimeConfig.hpp"
#include <iostream>
#include <exception>

int main(int argc, char **argv)
{
    try
    {
        const auto file = configArgument(argc, argv);
        if(file.empty())
        {
            return 0;
        }

        const auto config = RuntimeConfig::load(file);

        std::signal(SIGPIPE, SIG_IGN);

        EchoServer server(config);
        server.start();
        return 0;
    }
    catch(const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
