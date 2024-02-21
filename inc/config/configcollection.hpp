#include "configcollection.h"

namespace kmicki::config
{
    template<class T>
    T & ConfigCollection::AddConfig(ConfigFactory<T> factory)
    {
        static_assert(std::is_base_of_v<Config,T>, "Template parameter of AddConfig has to inherit kmicki::config::Config");
        auto config = factory(configuration);
        AddConfig(config);
        return *config;
    }
}