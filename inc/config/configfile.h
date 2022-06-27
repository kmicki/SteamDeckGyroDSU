#ifndef _KMICKI_CONFIG_CONFIGFILE_H_
#define _KMICKI_CONFIG_CONFIGFILE_H_

#include <string>
#include <vector>
#include <memory>

#include "config.h"

namespace kmicki::config
{
    class ConfigFile
    {
        public:

        ConfigFile() = delete;
        ConfigFile(std::string const& _filePath);

        bool LoadConfig(std::vector<std::unique_ptr<ConfigItemBase>> & configuration) const;
        bool SaveConfig(std::vector<std::unique_ptr<ConfigItemBase>> const& configuration, bool pretty = true) const;

        private:
        static const char cSeparator;
        std::string filePath;
    };
}

#endif