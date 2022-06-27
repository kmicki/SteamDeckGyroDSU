#ifndef _KMICKI_CONFIG_CONFIG_H_
#define _KMICKI_CONFIG_CONFIG_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace kmicki::config
{
    struct ConfigItemBase
    {
        std::string Name;
        virtual bool Update(std::string const& value,std::string & message) = 0;
        virtual std::string ValToString() const = 0;
    };

    using Data = std::vector<std::unique_ptr<ConfigItemBase>>;

    template<class T>
    struct ConfigItem : public ConfigItemBase
    {
        ConfigItem() = delete;
        ConfigItem(std::string const& name, T const& val);
        T Val; //value
        bool Update(std::string const& value,std::string & message) override;
        std::string ValToString() const override;
    };

    class Config
    {
        public:
        Config() = delete;
        Config(Data & _configData, std::string const& _prefix = "");
        Data & ConfigData();

        // called after configData vector is updated externally
        virtual void Load() = 0;

        // ensure configData vector has up-to-date values
        // values in configData can also be changed without this method being called
        virtual void Save() = 0;

        // set default values
        virtual void ToDefault() = 0;

        using SubscriptionCallback = std::function<void()>;

        void SubscribeToChange(SubscriptionCallback callback, void* marker = nullptr);
        void Unsubscribe(void* marker = nullptr);

        protected:
        std::vector<std::unique_ptr<ConfigItemBase>> & configData;

        template<class T>
        void SetValue(std::string const& name, T const& value);
        
        template<class T>
        T const* GetValuePtr(std::string const& name) const;

        template<class T>
        bool GetValue(std::string const& name, T& value) const;

        template<class T>
        bool GetValue(std::string const& name, T& value, bool & subscription) const;

        template<class T>
        T const& GetValue(std::string const& name) const;

        template<class T>
        ConfigItem<T> const* GetItem(std::string const& name) const; 
        
        void FireSubscriptions();

        private:

        std::string GetFullName(std::string const& name) const;

        std::string prefix;
        
        struct Subscription
        {
            void* marker;
            SubscriptionCallback callback;
        };

        std::vector<Subscription> subscriptions;

    };
}

#include "config.hpp"

#endif