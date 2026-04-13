#ifndef FG_ADDONMANAGER_HXX
#define FG_ADDONMANAGER_HXX

#include <string>
#include <map>
#include <memory>
#include <vector>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonVersion.hxx"

// NEW:
#include "AddonEngine.hxx"
#include "AddonEngineFactory.hxx"

namespace flightgear
{
namespace addons
{

class AddonManager
{
public:
    // NEW: declare constructor
    AddonManager();

    AddonManager(const AddonManager&) = delete;
    AddonManager& operator=(const AddonManager&) = delete;
    AddonManager(AddonManager&&) = delete;
    AddonManager& operator=(AddonManager&&) = delete;
    ~AddonManager() = default;

    static const std::unique_ptr<AddonManager>& createInstance();
    static const std::unique_ptr<AddonManager>& instance();
    static void reset();

    std::string registerAddon(const SGPath& addonPath);
    std::vector<AddonRef> registeredAddons() const;
    bool isAddonRegistered(const std::string& addonId) const;

    std::vector<AddonRef> loadedAddons() const;
    bool isAddonLoaded(const std::string& addonId) const;

    AddonRef getAddon(const std::string& addonId) const;
    AddonVersionRef addonVersion(const std::string& addonId) const;
    SGPath addonBasePath(const std::string& addonId) const;

    SGPropertyNode_ptr addonNode(const std::string& addonId) const;

    void addAddonMenusToFGMenubar() const;

    // NEW: drive all addon engines each frame
    void updateEngines(double dt);

    // NEW: shutdown all addon engines
    void shutdownEngines();

private:
    // REMOVE "= default" — we now define this in the .cxx file
    // explicit AddonManager() = default;

    static void loadConfigFileIfExists(const SGPath& configFile);
    std::string registerAddonMetadata(const SGPath& addonPath);

    std::map<std::string, AddonRef> _idToAddonMap;
    std::vector<AddonRef> _registeredAddons;
    int _loadSequenceNumber = 0;

    // NEW: runtime engine storage
    struct AddonRuntime {
        std::string id;
        std::string path;
        std::vector<std::unique_ptr<AddonEngine>> engines;
    };

    std::map<std::string, AddonRuntime> _runtime;

    // NEW: engine factory instance
    std::unique_ptr<AddonEngineFactory> _engineFactory;
};

} // namespace addons
} // namespace flightgear

#endif
