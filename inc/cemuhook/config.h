#ifndef _KMICKI_CEMUHOOK_CONFIG_H_
#define _KMICKI_CEMUHOOK_CONFIG_H_

#include "config/config.h"

namespace kmicki::cemuhook
{
    using ConfigItemBase = config::ConfigItemBase;
    using ConfigurationData = config::Data;

    template<class T>
    using ConfigItem = config::ConfigItem<T>;

    enum ConfigInterface
    {
        CfgIfLocal = 0,
        CfgIfAll
    };

    class Config : public config::Config
    {
        public:
        Config() = delete;
        Config(ConfigurationData & _configData, std::string prefix = "");

        ConfigInterface const& Interface() const;   // server.interface
        int const& Port() const;                    // server.port

        virtual void Load() override;
        virtual void Save() override;
        virtual void ToDefault() override;

        private:

        ConfigInterface interface;
        int port;
    };
}

#endif