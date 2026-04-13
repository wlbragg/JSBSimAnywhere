#pragma once

#include <string>
#include <vector>
#include <memory>

#include <simgear/misc/sg_path.hxx>
#include <FDM/JSBSim/FGFDMExec.h>
#include "PropertyBindings.hxx"

namespace flightgear {
namespace addons {

class FDMManager
{
public:
    FDMManager(const std::string& addonId,
               const SGPath& addonPath,
               const std::string& model,
               const std::string& root,
               bool disableDefault,
               std::vector<std::string> publishList,
               const std::string& mode,
               const std::string& pathOverride,
               bool debug);

    void init();
    void step(double dt);
    void shutdown();

private:
    void syncInputs();
    void syncOutputs();
    void applyBindings();

    std::vector<PropertyBinding> _bindings;
    std::unique_ptr<JSBSim::FGFDMExec> _jsb;

    std::string _addonId;
    SGPath _addonPath;

    std::string _fdmModel;
    std::string _fdmRoot;
    bool _disableDefault;
    std::vector<std::string> _publishList;
    std::string _mode;
    std::string _fdmPath;
    bool _debug;
};

} // namespace addons
} // namespace flightgear
