#pragma once

#include "simplifier_interface.hpp"
#include <memory>
#include <functional>

namespace MeshSimplify 
{


class SimplifierRegistry 
{
public:
    using Creator = std::function<std::unique_ptr<ISimplifier>()>;
   
    static SimplifierRegistry& instance() 
    {
        static SimplifierRegistry registry;
        return registry;
    }
   
    void registerSimplifier(const std::string& name, Creator creator) 
    {
        creators_[name] = std::move(creator);
    }
    

    std::unique_ptr<ISimplifier> create(const std::string& name) const 
    {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr;
    }

    std::vector<std::string> getNames() const 
    {
        std::vector<std::string> names;
        names.reserve(creators_.size());
        for (const auto& [name, _] : creators_)
            names.push_back(name);
        return names;
    }
    

    bool has(const std::string& name) const 
    {
        return creators_.find(name) != creators_.end();
    }

    size_t size() const { return creators_.size(); }
    
private:
    SimplifierRegistry() = default;
    ~SimplifierRegistry() = default;
    
    SimplifierRegistry(const SimplifierRegistry&) = delete;
    SimplifierRegistry& operator=(const SimplifierRegistry&) = delete;
    
    std::map<std::string, Creator> creators_;
};

}

#define REGISTER_SIMPLIFIER(ClassName, DisplayName) \
    namespace { \
        struct ClassName##_Registrar { \
            ClassName##_Registrar() { \
                MeshSimplify::SimplifierRegistry::instance().registerSimplifier( \
                    DisplayName, []() { return std::make_unique<ClassName>(); }); \
            } \
        }; \
        static ClassName##_Registrar ClassName##_Registrar_instance; \
    }
    