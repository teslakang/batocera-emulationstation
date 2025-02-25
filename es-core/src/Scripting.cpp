#include "Scripting.h"
#include "Log.h"
#include "utils/Platform.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/VectorEx.h"
#include "Paths.h"
#include <thread>
#include <set>
#include <map>

using namespace Utils::Platform;

namespace Scripting
{
    static void executeScript(const std::string& script, const std::string& eventName, const std::string& arg1, const std::string& arg2, const std::string& arg3)
    {
        std::string command = script;

        if (!eventName.empty())
            command += " " + eventName;

        for (auto arg : { arg1, arg2, arg3 })
        {
            if (arg.empty())
                break;

            command += " \"" + arg + "\"";
        }

#if WIN32
        LOG(LogDebug) << "  executing: " << script;

        // Start using a thread to avoid lags

        auto runScript = [command, eventName]() {
            ProcessStartInfo psi;
            psi.command = command;
            psi.waitForExit = (eventName == "quit");
            psi.showWindow = false;
            psi.run();
        };

        std::thread runThread(runScript);
        runThread.detach();
#else            
        LOG(LogDebug) << "  executing: " << script;

        ProcessStartInfo psi;
        psi.command = command;
        psi.waitForExit = (eventName == "quit");
        psi.showWindow = false;
        psi.run();
#endif
    }

    static std::set<std::string> _supportedExtensions = { ".exe", ".cmd", ".bat", ".ps1", ".sh", ".py" };

    void fireEvent(const std::string& eventName, const std::string& arg1, const std::string& arg2, const std::string& arg3)
    {
        LOG(LogDebug) << "fireEvent: " << eventName << " " << arg1 << " " << arg2 << " " << arg3;

        // Process splitted paths scripts
        std::vector<std::string> scriptDirList =
        {
            Paths::getUserEmulationStationPath() + "/scripts/" + eventName,
            Paths::getEmulationStationPath() + "/scripts/" + eventName,
#ifndef WIN32
            "/var/run/emulationstation/scripts/" + eventName
#endif
        };

        for (auto dir : VectorHelper::distinct(scriptDirList, [](auto x) { return x; }))
        {
            auto scripts = Utils::FileSystem::getDirContent(dir);
            for (auto script : scripts)
            {
#if WIN32
                auto ext = Utils::String::toLower(Utils::FileSystem::getExtension(script));
                if (_supportedExtensions.find(ext) == _supportedExtensions.cend())
                    continue;
#endif
                executeScript(script, "", arg1, arg2, arg3);
            }
        }

        // Process single scripts. This type of scripts are called with the event name as 1st arg
        std::vector<std::string> paths =
        {
            Paths::getUserEmulationStationPath() + "/scripts",
            Paths::getEmulationStationPath() + "/scripts",
#ifndef WIN32
            "/var/run/emulationstation/scripts"
#endif
        };

        for (auto dir : VectorHelper::distinct(paths, [](auto x) { return x; }))
        {
            if (!Utils::FileSystem::exists(dir))
                continue;

            for (auto script : Utils::FileSystem::getDirectoryFiles(dir))
            {
                if (script.directory)
                    continue;

                auto ext = Utils::String::toLower(Utils::FileSystem::getExtension(script.path));
                if (_supportedExtensions.find(ext) == _supportedExtensions.cend())
                    continue;

                executeScript(script.path, eventName, arg1, arg2, arg3);
            }
        }
    }
} // Scripting::
