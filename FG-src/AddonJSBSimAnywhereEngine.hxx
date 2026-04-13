#pragma once

#include "AddonEngine.hxx"

#include <FDM/JSBSim/FGFDMExec.h>
#include <simgear/misc/sg_path.hxx>

#include <memory>
#include <string>
#include <vector>

namespace flightgear {
namespace addons {

class AddonJSBSimAnywhereEngine : public AddonEngine
{
public:
    AddonJSBSimAnywhereEngine(const std::string& addonId,
                              const std::string& addonPath);

    void init() override;
    void update(double dt) override;
    std::string getName() const override { return "AddonJSBSimAnywhereEngine"; }
    //void shutdown() override;

private:
    // --------------------------------------------------------------------
    // Property configuration
    // --------------------------------------------------------------------
    void configureFromProperties();
    void loadCompatibilityConfig();

    // --------------------------------------------------------------------
    // Addon identity
    // --------------------------------------------------------------------
    std::string _addonId;
    std::string _addonPath;

    // --------------------------------------------------------------------
    // JSBSim engine instance
    // --------------------------------------------------------------------
    std::unique_ptr<JSBSim::FGFDMExec> _jsb;

    // --------------------------------------------------------------------
    // Publish list from <publish> block
    // --------------------------------------------------------------------
    std::vector<std::string> _publishProps;

    // --------------------------------------------------------------------
    // FDM configuration
    // --------------------------------------------------------------------
    std::string _mode;        // "benchmark" or "aircraft"
    std::string _fdmModel;    // model name (ASK13, F16-1, etc.)
    std::string _fdmPath;     // external aircraft path (empty = internal)

    bool _debug = false;

    // --------------------------------------------------------------------
    // Compatibility scan flags
    // --------------------------------------------------------------------
    bool _compatibilityEnabled   = false;
    bool _compatibilityFullScan  = false;
    bool _compatibilityLogAll    = false;

    // --------------------------------------------------------------------
    // Compatibility scan + tree builder
    // --------------------------------------------------------------------
    bool runCompatibilityScan(const SGPath& aircraftRoot,
                              bool fullScan,
                              bool logAll);

    // --------------------------------------------------------------------
    // XML file collection helpers
    // --------------------------------------------------------------------
    void collectAllXMLFiles(const SGPath& root,
                            std::vector<SGPath>& out);

    void collectXMLFilesInDirectory(const SGPath& dir,
                                    std::vector<SGPath>& out);

    struct PropertyBinding {
    std::string path;
        SGPropertyNode* fgNode;              // FG global tree node
        JSBSim::FGPropertyNode* jsbNode;     // JSBSim property node
        enum Direction { FG_TO_JSB, JSB_TO_FG } dir;
    };

    std::vector<PropertyBinding> _bindings;

    SGPropertyNode_ptr _numEnginesNode;
    SGPropertyNode_ptr _eng0_rpm;
    SGPropertyNode_ptr _eng0_starter_rpm;
    SGPropertyNode_ptr _eng0_running;
    SGPropertyNode_ptr _eng0_throttle_norm;
    SGPropertyNode_ptr _eng0_mixture_norm;
    SGPropertyNode_ptr _eng0_prop_rpm;
};

} // namespace addons
} // namespace flightgear


