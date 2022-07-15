#include "config/config.h"
#include "log/log.h"

#include <sstream>
#include <algorithm>

namespace kmicki::config
{
    ConfigItemBase::ConfigItemBase(std::string name, ConfigComment comment)
    : Name(name), Comment(comment)
    {}
    
    template<>
    bool ConfigItem<std::string>::Update(std::string const& value,std::string & message)
    {
        Val = value;
        std::ostringstream str;
        str << "existing item value updated (type: string), Name=" << Name << " Value=" << value;
        message = str.str();
        return true;
    }

    template<>
    bool ConfigItem<int>::Update(std::string const& value,std::string & message)
    {
        int val;
        try
        {
            val = std::stoi(value);
        }
        catch(const std::invalid_argument& e)
        {
            std::ostringstream str;
            str << "invalid type (should be int), Name=" << Name << " Value=" << value;
            message = str.str();
            return false;
        }
        catch(const std::out_of_range& e)
        {
            std::ostringstream str;
            str << "value out of range (type: int), Name=" << Name << " Value=" << value;
            message = str.str();
            return false;
        }

        std::ostringstream str;
        str << "existing item value updated (type: int), Name=" << Name << " Value=" << val;
        message = str.str();
        Val = val;
        return true;
    }

    template<>
    bool ConfigItem<bool>::Update(std::string const& value,std::string & message)
    {
        std::string strVal(value.length(),' ');
        std::transform(value.begin(),value.end(),strVal.begin(),[](char c){ return std::tolower(c); });
        bool val;
        if(value == "true" || value == "1" || value == "yes" || value == "t" || value == "y")
            val = true;
        else if(value == "false" || value == "0" || value == "no" || value == "f" || value == "n")
            val = false;
        else
        {
            std::ostringstream str;
            str << "invalid type (should be bool), Name=" << Name << " Value=" << value;
            message = str.str();
            return false;
        }

        std::ostringstream str;
        str << "existing item value updated (type: bool), Name=" << Name << " Value=" << val;
        message = str.str();
        Val = val;
        return true;
    }

    template<>
    std::string ConfigItem<std::string>::ValToString() const
    {
        return Val;
    }

    template<>
    std::string ConfigItem<int>::ValToString() const
    {
        return std::to_string(Val);
    }

    template<>
    std::string ConfigItem<bool>::ValToString() const
    {
        return Val?"true":"false";
    }

    template struct ConfigItem<std::string>;
    template struct ConfigItem<int>;
    template struct ConfigItem<bool>;

    Config::Config(std::vector<std::unique_ptr<ConfigItemBase>> & _configData, std::string const& _prefix)
    : configData(_configData), prefix(_prefix), subscriptions()
    { }

    std::vector<std::unique_ptr<ConfigItemBase>> & Config::ConfigData()
    {
        return configData;
    }

    void Config::FireSubscriptions()
    {
        for(auto& sub : subscriptions)
            sub.callback();
    }    
    
    void Config::SubscribeToChange(SubscriptionCallback callback, void* marker)
    {
        // TODO: change callback to std::function
        if(callback == nullptr)
            return;

        Unsubscribe(marker);

        auto &sub = subscriptions.emplace_back();
        sub.callback = callback;
        sub.marker = marker;
    }

    void Config::Unsubscribe(void* marker)
    {
        if(subscriptions.empty())
            return;
        auto sub = std::find_if(subscriptions.cbegin(),subscriptions.cend(),[&](auto const& x){ return x.marker == marker; });
        if(sub == subscriptions.end())
            return;
        
        subscriptions.erase(sub);
    }

    std::string Config::GetFullName(std::string const& name) const
    {
        return prefix.empty()?name:(prefix + "." + name);
    }
}