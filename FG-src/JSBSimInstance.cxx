#include "JSBSimInstance.hxx"
#include "PropertyTreeBuilder.hxx"

#include <FDM/JSBSim/FGFDMExec.h>
#include <JSBSim/models/FGModel.h>
#include <JSBSim/input_output/FGPropertyManager.h>

#include <Main/fg_props.hxx>

JSBSimInstance::JSBSimInstance(const std::string& id,
                               const std::string& modelPath)
    : _id(id), _modelPath(modelPath)
{
    // Build FG-side property tree
    PropertyTreeBuilder::createTree(id);

    // Create JSBSim engine
    _fdm = std::make_unique<FGFDMExec>();
}

JSBSimInstance::~JSBSimInstance() = default;

bool JSBSimInstance::loadModel()
{
    return _fdm->LoadModel(_modelPath);
}

void JSBSimInstance::bindToFGProperties()
{
    std::string base = "/addons/jsbsim/instances/" + _id;

    // Bind JSBSim to FG property tree
    _fdm->GetPropertyManager()->Tie("position/lat-gc-deg",
        fgGetNode((base + "/position/lat-gc-deg").c_str(), true));

    _fdm->GetPropertyManager()->Tie("position/long-gc-deg",
        fgGetNode((base + "/position/long-gc-deg").c_str(), true));

    _fdm->GetPropertyManager()->Tie("position/altitude-ft",
        fgGetNode((base + "/position/altitude-ft").c_str(), true));

    // Add more ties as needed
}

void JSBSimInstance::update(double dt)
{
    _fdm->Run(dt);
}

double JSBSimInstance::get(const std::string& path) const
{
    std::string full = "/addons/jsbsim/instances/" + _id + "/" + path;
    return fgGetDouble(full.c_str());
}

void JSBSimInstance::set(const std::string& path, double value)
{
    std::string full = "/addons/jsbsim/instances/" + _id + "/" + path;
    fgSetDouble(full.c_str(), value);
}
