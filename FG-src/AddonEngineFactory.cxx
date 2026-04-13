#include "AddonEngineFactory.hxx"
#include "AddonJSBSimAnywhereEngine.hxx"

#include <simgear/debug/logstream.hxx>

namespace flightgear {
namespace addons {

std::unique_ptr<AddonEngine>
AddonEngineFactory::create(const std::string& p1,
                           const std::string& p2,
                           const std::string& p3)
{
    // Correct mapping based on actual logs:
    const std::string& engineName = p1;   // "AddonJSBSimAnywhereEngine"
    const std::string& addonId    = p2;   // "org.flightgear.addons.JSBSimAnywhere"
    const std::string& addonPath  = p3;   // "/media/.../JSBSimAnywhere"

    SG_LOG(SG_GENERAL, SG_ALERT,
           "AddonEngineFactory::create engineName=" << engineName
           << " addonId=" << addonId
           << " addonPath=" << addonPath);

    if (engineName == "AddonJSBSimAnywhereEngine" &&
        addonId == "org.flightgear.addons.JSBSimAnywhere")
    {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AddonEngineFactory::create -> constructing AddonJSBSimAnywhereEngine");

        return std::make_unique<AddonJSBSimAnywhereEngine>(addonId, addonPath);
    }

    return nullptr;
}

} // namespace addons
} // namespace flightgear
