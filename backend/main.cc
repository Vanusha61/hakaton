#include <filesystem>

#include <drogon/drogon.h>

namespace
{
std::string resolveConfigPath(int argc, char *argv[])
{
    if (argc > 1)
    {
        return argv[1];
    }

    if (std::filesystem::exists("config.json"))
    {
        return "config.json";
    }

    return "../config.json";
}
}  // namespace

int main(int argc, char *argv[])
{
    drogon::app().loadConfigFile(resolveConfigPath(argc, argv));
    drogon::app().run();
    return 0;
}
