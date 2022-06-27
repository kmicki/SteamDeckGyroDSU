#include "cemuhook/config.h"
#include <sstream>
#include <algorithm>

namespace kmicki::config
{
    using ConfigInterface = cemuhook::ConfigInterface;
    static const int cConfigInterfaceEnumLen = 2;
    static std::string cConfigInterfaceStrVals[cConfigInterfaceEnumLen] = { "local" , "all" };

    template<>
    bool ConfigItem<ConfigInterface>::Update(std::string const& value,std::string & message)
    {

        std::string strVal(value.length(),' ');
        std::transform(value.begin(),value.end(),strVal.begin(),[](char c){ return std::tolower(c); });
        for (int i = 0; i < cConfigInterfaceEnumLen; ++i)
            if (strVal == cConfigInterfaceStrVals[i])
            {
                Val = (ConfigInterface)i;
                std::ostringstream str;
                str << "existing item value updated (type: network-interface), Name=" << Name << " Value=" << strVal;
                message = str.str();
                return true;
            }

        std::ostringstream str;
        str << "value invalid (type: network-interface), Name=" << Name << " Value=" << value;
        message = str.str();
        return false;
    }

    template<>
    std::string ConfigItem<ConfigInterface>::ValToString() const
    {
        if(Val >= 0 && Val <= cConfigInterfaceEnumLen)
            return cConfigInterfaceStrVals[Val];
        return "";
    }

    template struct ConfigItem<ConfigInterface>;
}

namespace kmicki::cemuhook
{
    static const std::string cPrefix = "server";

    // Config items

    static const std::string cInterfaceStr = "interface";
    static const ConfigInterface cInterfaceDefault = CfgIfAll;

    static const std::string cPortStr = "port";
    static const int cPortDefault = 26760;

    Config::Config(std::vector<std::unique_ptr<ConfigItemBase>> & _configData, std::string prefix)
    : config::Config(_configData, (prefix.empty() ? "" : (prefix + ".")) + cPrefix)
    {
        ToDefault();
        Load();
    }

    ConfigInterface const& Config::Interface() const
    {
        return interface;
    }

    int const& Config::Port() const
    {
        return port;
    }

    void Config::Load()
    {
        bool subscription = false;
        GetValue(cInterfaceStr,interface,subscription);
        GetValue(cPortStr,port,subscription);
        if(subscription)
            FireSubscriptions();
    }

    void Config::Save()
    {
        SetValue(cInterfaceStr,interface);
        SetValue(cPortStr,port);
    }

    void Config::ToDefault()
    {
        interface = cInterfaceDefault;
        port = cPortDefault;
    }
}