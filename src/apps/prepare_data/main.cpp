#include "dataPreparation.hpp"
#include "runtimeConfig.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
    try
    {
        const auto file = configArgument(argc, argv);
        if(file.empty())
        {
            return 0;
        }

        prepareData(RuntimeConfig::load(file));
        return 0;
    }
    catch(const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
