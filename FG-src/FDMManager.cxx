#include "FDMManager.hxx"
#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>
#include <simgear/misc/sg_path.hxx>
#include "PropertyBindings.hxx"

using namespace JSBSim;

namespace flightgear {
namespace addons {

FDMManager::FDMManager(const std::string& addonId,
                       const SGPath& addonPath,
                       const std::string& model,
                       const std::string& root,
                       bool disableDefault,
                       std::vector<std::string> publishList,
                       const std::string& mode,
                       const std::string& pathOverride,
                       bool debug)
    : _addonId(addonId),
      _addonPath(addonPath),
      _fdmModel(model),
      _fdmRoot(root),
      _disableDefault(disableDefault),
      _publishList(std::move(publishList)),
      _mode(mode),
      _fdmPath(pathOverride),
      _debug(debug)
{
}

void FDMManager::init()
{
    // Create JSBSim instance
    _jsb = std::make_unique<FGFDMExec>();

    // Benchmark mode path resolution
    SGPath root = _addonPath / _fdmRoot;                     // <addon>/JSBSim
    SGPath aircraftPath = root / "aircraft" / _fdmModel;     // <addon>/JSBSim/aircraft/benchmark
    SGPath enginePath  = aircraftPath / "Engines";
    SGPath systemsPath = aircraftPath / "Systems";

    // Debug path output
    if (_debug) {
        SG_LOG(SG_GENERAL, SG_ALERT, "=== JSBSim PATH DEBUG START ===");
        SG_LOG(SG_GENERAL, SG_ALERT, "addonPath     = " << _addonPath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "rootDir       = " << root.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "aircraftPath  = " << aircraftPath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "enginePath    = " << enginePath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "systemsPath   = " << systemsPath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "modelName     = " << _fdmModel);
        SG_LOG(SG_GENERAL, SG_ALERT, "=== JSBSim PATH DEBUG END ===");
    }

    // Set JSBSim search paths
    _jsb->SetRootDir(root.str());
    _jsb->SetAircraftPath(aircraftPath.str());

    // Load model using 5‑argument API
    if (!_jsb->LoadModel(
            aircraftPath.str(),   // model directory
            enginePath.str(),     // Engines/
            systemsPath.str(),    // Systems/
            _fdmModel,            // model name
            false))               // do not use FG defaults
    {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Addon JSBSim: LoadModel() failed for " << aircraftPath.str());
        return;
    }

    SG_LOG(SG_GENERAL, SG_ALERT, "JSBSim: LoadModel() succeeded");

    if (!_jsb->RunIC()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "JSBSim: RunIC() failed");
        return;
    }

    SG_LOG(SG_GENERAL, SG_INFO, "JSBSim addon FDM initialized successfully");
}

void FDMManager::syncInputs()
{
    for (const auto& p : _publishList)
    {
        std::string fgPath = "/addon/fdm/jsbsim/" + p;
        double val = fgGetDouble(fgPath.c_str());
        _jsb->SetPropertyValue(p, val);
    }
}

void FDMManager::syncOutputs()
{
    for (const auto& p : _publishList)
    {
        double val = _jsb->GetPropertyValue(p);
        std::string fgPath = "/addon/fdm/jsbsim/" + p;
        fgSetDouble(fgPath.c_str(), val);
    }
}

void FDMManager::applyBindings()
{
    for (const auto& b : _bindings)
    {
        if (b.dir == PropertyBinding::FG_TO_JSB)
        {
            double v = b.fgNode->getDoubleValue();
            b.jsbNode->setDoubleValue(v);
        }
    }
}

void FDMManager::step(double dt)
{
    applyBindings();
    //syncInputs();
    _jsb->Run();
    syncOutputs();

    if (_debug)
    {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSim step: math=" << _jsb->GetPropertyValue("fcs/benchmark/math-output")
               << " pid=" << _jsb->GetPropertyValue("fcs/benchmark/pid-output")
               << " filt=" << _jsb->GetPropertyValue("fcs/benchmark/filter-output"));
    }
}

void FDMManager::shutdown()
{
    _jsb.reset();
}

} // namespace addons
} // namespace flightgear
