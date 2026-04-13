#pragma once

#include <memory>
#include <string>

namespace flightgear {
namespace addons {

class AddonEngine;

class AddonEngineFactory
{
public:
    static std::unique_ptr<AddonEngine>
    create(const std::string& addonId,
           const std::string& addonPath,
           const std::string& engineName);
};

} // namespace addons
} // namespace flightgear
