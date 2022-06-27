#ifndef _KMICKI_SDGYRODSU_CONFIG_H_
#define _KMICKI_SDGYRODSU_CONFIG_H_

#include "config/config.h"

namespace kmicki::sdgyrodsu
{
    typedef config::ConfigItemBase ConfigItemBase;
    template<class T>
    using ConfigItem = config::ConfigItem<T>;

    class Config : public config::Config
    {
        public:
        Config() = delete;
        Config(std::vector<std::unique_ptr<ConfigItemBase>> & _configData);

        virtual void Load();
        virtual void Save();
    };
}

#endif