#include "config.h"
#include <stdexcept>
#include <algorithm>

namespace kmicki::config
{
    template<class T>
    ConfigItem<T>::ConfigItem(std::string const& name, T const& val, ConfigComment const& comment)
    : ConfigItemBase(name,comment), Val(val)
    { }

    template<class T>
    void Config::SetValue(std::string const& name, T const& value, ConfigComment const& defaultComment)
    {
        auto fullName = GetFullName(name);
        ConfigItem<T>* itemPtr;

        auto item = std::find_if(configData.begin(),configData.end(),
            [&](std::unique_ptr<ConfigItemBase> const& x)
                { return x->Name == fullName && (itemPtr = dynamic_cast<ConfigItem<T>*>(x.get())) != nullptr; });
        
        if(item == configData.end())
        {
            configData.emplace_back(new ConfigItem(fullName,value,defaultComment));
            return;
        }

        itemPtr->Val = value;
    }

    template<class T>
    ConfigItem<T> const* Config::GetItem(std::string const& name) const
    {
        auto fullName = GetFullName(name);
        ConfigItem<T> const* itemPtr;

        auto item = std::find_if(configData.begin(),configData.end(),
            [&](std::unique_ptr<ConfigItemBase> const& x)
                { return x->Name == fullName && (itemPtr = dynamic_cast<ConfigItem<T> const*>(x.get())) != nullptr; });
        
        if(item == configData.end())
        {
            return nullptr;
        }
        
        return itemPtr;
    }

    template<class T>
    T const* Config::GetValuePtr(std::string const& name) const
    {
        auto item = GetItem<T>(name);
        if(item == nullptr)
            return nullptr;

        return &(item->Val);
    }

    template<class T>
    bool Config::GetValue(std::string const& name, T& value) const
    {
        auto newVal = GetValuePtr<T>(name);
        if(newVal == nullptr)
            return false;

        value = *newVal;
        return true;
    }

    template<class T>
    bool Config::GetValue(std::string const& name, T& value, bool & subscription) const
    {
        auto newVal = GetValuePtr<T>(name);
        if(newVal == nullptr)
            return false;

        if(value != *newVal)
            subscription = true;

        value = *newVal;
        return true;
    }

    template<class T>
    T const& Config::GetValue(std::string const& name) const
    {
        auto value = GetValuePtr<T>(name);
        if(value == nullptr)
            throw std::invalid_argument("name");

        return *value;
    }
}