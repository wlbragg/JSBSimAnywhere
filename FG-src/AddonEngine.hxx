#pragma once

#include <string>

namespace flightgear {
namespace addons {

class AddonEngine
{
public:
    virtual ~AddonEngine() = default;

    virtual void init() {}
    virtual void update(double dt) {}
    virtual void shutdown() {}

    // NEW: allow AddonManager to pass FDM configuration to engines
    // Default implementation does nothing; specific engines override it.
    virtual void setFDMConfig(bool disableDefault,
                              const std::string& model,
                              const std::string& root) {}

    virtual std::string getName() const = 0;
};

} // namespace addons
} // namespace flightgear
