#include "config/configcollection.h"
#include "log/log.h"

using namespace kmicki::log;

namespace kmicki::config
{
    ConfigCollection::ConfigCollection(std::string const& filePath)
        : Config(configuration),configuration(),configs(),file(filePath)
    { }

    void ConfigCollection::AddConfig(Config * config)
    {
        configs.emplace_back(config);
        config->SubscribeToChange([&] { subscription = true; });
    }

    void ConfigCollection::Load()
    {
        subscription = false;
        for(auto& conf : configs)
            conf->Load();
        
        if(subscription)
            FireSubscriptions();
    }

    void ConfigCollection::Save()
    {
        for(auto& conf : configs)
            conf->Save();
    }

    void ConfigCollection::ToDefault()
    {
        for(auto& conf : configs)
            conf->ToDefault();
    }

    bool ConfigCollection::LoadFromFile()
    {
        if(file.LoadConfig(configuration))
        {
            Load();
            return true;
        }
        return false;
    }

    bool ConfigCollection::SaveToFile()
    {
        Save();
        return file.SaveConfig(configuration);
    }

    bool ConfigCollection::Initialize()
    {
        ToDefault();
        Save();
        LoadFromFile();
        return SaveToFile();
    }
}