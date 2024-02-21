#ifndef _KMICKI_CONFIG_CONFIGCOLLECTION_
#define _KMICKI_CONFIG_CONFIGCOLLECTION_

#include <vector>
#include <memory>
#include <functional>
#include "config.h"
#include "configfile.h"

namespace kmicki::config
{
    class ConfigCollection : public Config
    {
        public:

        template<class T>
        using ConfigFactory = std::function<T*(Data &)>; 

        ConfigCollection() = delete;

        // Create config collection stored in a file
        ConfigCollection(std::string const& filePath);

        // Takes ownership of the provided config
        void AddConfig(Config * config);

        template<class T>
        T & AddConfig(ConfigFactory<T> factory);

        virtual void Load() override;
        virtual void Save() override;
        virtual void ToDefault() override;

        virtual bool LoadFromFile();
        virtual bool SaveToFile();

        virtual bool Initialize();

        protected:
        Data configuration;

        std::vector<std::unique_ptr<Config>> configs;
        ConfigFile file;

        private:
        bool subscription;

    };
}

#include "configcollection.hpp"

#endif