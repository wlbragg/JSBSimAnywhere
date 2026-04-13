#include "AddonJSBSimAnywhereEngine.hxx"

#include "PropertyTreeScanner.hxx"

#include "FDM/JSBSim/models/FGPropulsion.h"

#include <Main/fg_props.hxx>

#include <dirent.h>
#include <sys/stat.h>
#include <cstring>


namespace flightgear {
namespace addons {

AddonJSBSimAnywhereEngine::AddonJSBSimAnywhereEngine(const std::string& addonId,
                                                     const std::string& addonPath)
    : AddonEngine(),          // base has only default ctor
      _addonId(addonId),
      _addonPath(addonPath)
{
    SG_LOG(SG_GENERAL, SG_ALERT,
           "AddonJSBSimAnywhereEngine::ctor addonId=" << _addonId
           << " path=" << _addonPath);
}

static void dumpNode(SGPropertyNode* node)
{
    if (!node) return;

    SG_LOG(SG_GENERAL, SG_ALERT,
           node->getPath() << " = " << node->getStringValue());

    for (int i = 0; i < node->nChildren(); i++)
    {
        SGPropertyNode* child = node->getChild(i);
        if (child)
            dumpNode(child);
    }
}

void AddonJSBSimAnywhereEngine::configureFromProperties()
{
    SG_LOG(SG_GENERAL, SG_ALERT,
           "AddonJSBSimAnywhereEngine::configureFromProperties() CALLED");

    // Base node: /addon/fdm/jsbsim
    SGPropertyNode* jsbNode =
        fgGetNode("/addon/fdm/jsbsim", false);

    if (!jsbNode) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSimAnywhere: Missing /addon/fdm/jsbsim node");
        return;
    }

    // ------------------------------------------------------------
    // Debug flag
    // ------------------------------------------------------------
    if (auto* dbgNode = jsbNode->getChild("debug"))
        _debug = dbgNode->getBoolValue(false);
    else
        _debug = false;

    // ------------------------------------------------------------
    // fdm-control block
    // ------------------------------------------------------------
    SGPropertyNode* ctrlNode = jsbNode->getChild("fdm-control");
    if (!ctrlNode) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSimAnywhere: Missing <fdm-control> block");
        return;
    }

    // mode: "benchmark" or "aircraft"
    if (auto* modeNode = ctrlNode->getChild("mode"))
        _mode = modeNode->getStringValue();
    else
        _mode = "benchmark";   // default

    // addon-model: required for both modes
    if (auto* modelNode = ctrlNode->getChild("addon-model"))
        _fdmModel = modelNode->getStringValue();
    else
        _fdmModel = "benchmark";   // default

    // path: only used in aircraft mode
    if (auto* pathNode = ctrlNode->getChild("path"))
        _fdmPath = pathNode->getStringValue();
    else
        _fdmPath.clear();

    // ------------------------------------------------------------
    // Publish list (optional)
    // ------------------------------------------------------------
    _publishProps.clear();

    if (auto* pubNode = jsbNode->getChild("publish")) {
        for (int i = 0; i < pubNode->nChildren(); ++i) {
            SGPropertyNode* p = pubNode->getChild(i);
            if (!p) continue;

            if (p->getNameString() == "property") {
                std::string prop = p->getStringValue();
                if (!prop.empty())
                    _publishProps.push_back(prop);
            }
        }
    }

    // ------------------------------------------------------------
    // Debug summary
    // ------------------------------------------------------------
    if (_debug) {
        SG_LOG(SG_GENERAL, SG_ALERT, "JSBSimAnywhere: mode=" << _mode
               << " model=" << _fdmModel
               << " path=" << _fdmPath
               << " publishCount=" << _publishProps.size());
    }
}

void AddonJSBSimAnywhereEngine::loadCompatibilityConfig()
{
    // /addons/by-id/<id>/addon-local/compatibility
    std::string basePath =
        "/addons/by-id/" + _addonId + "/compatibility";

    SGPropertyNode* base = fgGetNode(basePath, false);

    if (!base) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSimAnywhere: no compatibility config at " << basePath);
        return;
    }

    _compatibilityEnabled  = base->getBoolValue("enabled", false);
    _compatibilityFullScan = base->getBoolValue("full-scan", false);
    _compatibilityLogAll   = base->getBoolValue("log-all", false);

    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSBSimAnywhere: compatibility config: enabled="
           << _compatibilityEnabled
           << " full-scan=" << _compatibilityFullScan
           << " log-all=" << _compatibilityLogAll);
}

bool AddonJSBSimAnywhereEngine::runCompatibilityScan(
        const SGPath& aircraftRoot,
        bool fullScan,
        bool logAll)
{
    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSBSimAnywhere: compatibility scan (fullScan="
           << fullScan << ", logAll=" << logAll << ")");

    if (!_jsb) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Compatibility scan: JSBSim exec is null, skipping");
        return false;
    }

    std::vector<SGPath> xmlFiles;

    if (fullScan) {
        collectAllXMLFiles(aircraftRoot, xmlFiles);
    }

    const std::vector<std::string> setNames = {
        _fdmModel + "-set.xml",
        _fdmModel + "-main.xml",
        _fdmModel + ".set",
        _fdmModel + ".main.xml",
        _fdmModel + "_set.xml",
        _fdmModel + "_main.xml"
    };

    for (const auto& name : setNames) {
        SGPath p = aircraftRoot / name;
        if (p.exists()) {
            xmlFiles.push_back(p);
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "COMPAT: found -set/-main file: " << p.str());
        }
    }

    collectXMLFilesInDirectory(aircraftRoot / "Systems",  xmlFiles);
    collectXMLFilesInDirectory(aircraftRoot / "systems",  xmlFiles);
    collectXMLFilesInDirectory(aircraftRoot / "controls", xmlFiles);
    collectXMLFilesInDirectory(aircraftRoot / "Controls", xmlFiles);
    collectXMLFilesInDirectory(aircraftRoot / "FCS",      xmlFiles);
    collectXMLFilesInDirectory(aircraftRoot / "fcs",      xmlFiles);

    SG_LOG(SG_GENERAL, SG_ALERT,
           "COMPAT: collected " << xmlFiles.size() << " XML files");

    if (xmlFiles.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Compatibility scan: no XML files found under "
               << aircraftRoot.str());
        return false;
    }

    // ------------------------------------------------------------
    // 1) FG-side scan: systems, controls, FCS, set/main, etc.
    //    We only skip the *actual* JSBSim FDM file here.
    // ------------------------------------------------------------
    PropertyTreeScanner scanner;

    // First, identify the JSBSim FDM file path (if present)
    SGPath fdmPath;
    {
        for (const auto& f : xmlFiles) {
            std::string p = f.str();

            // Must be under <aircraftRoot>/JSBSim/<model>/ or just <aircraftRoot>/<model>.xml
            // depending on how aircraftRoot is passed in.
            // We keep this simple: match by filename and avoid Systems/Engines/etc.
            if (f.file() != (_fdmModel + ".xml"))
                continue;

            if (p.find("/Systems/")  != std::string::npos ||
                p.find("/systems/")  != std::string::npos ||
                p.find("/Engines/")  != std::string::npos ||
                p.find("/engines/")  != std::string::npos ||
                p.find("/Models/")   != std::string::npos ||
                p.find("/models/")   != std::string::npos ||
                p.find("/Nasal/")    != std::string::npos ||
                p.find("/nasal/")    != std::string::npos ||
                p.find("/gui/")      != std::string::npos)
                continue;

            fdmPath = f;
            break;
        }
    }

    for (const auto& f : xmlFiles) {
        // Skip only the *one* FDM file we will handle with jsbScanner
        if (!fdmPath.isNull() && f == fdmPath) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "SKIP FG scan for JSBSim FDM file: " << f.str());
            continue;
        }

        SG_LOG(SG_GENERAL, SG_ALERT, "FG SCAN: " << f.str());
        scanner.scanFGFile(f.str());
    }

    // ------------------------------------------------------------
    // 2) JSBSim FDM scan: internal FDM properties
    // ------------------------------------------------------------
    PropertyTreeScanner jsbScanner;

    if (!fdmPath.isNull()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "SCAN JSBSim FDM: " << fdmPath.str());
        jsbScanner.scanJSBSimFile(fdmPath.str());
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "ERROR: Could not locate JSBSim FDM file for model " << _fdmModel);
    }

    // Merge FG + JSBSim properties
    std::set<std::string> mergedProps = scanner.getFGProperties();
    const auto& jsbProps = jsbScanner.getJSBSimProperties();
    mergedProps.insert(jsbProps.begin(), jsbProps.end());

    if (mergedProps.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Compatibility scan: no property references found");
        return true;
    }

    SGPropertyNode* fgRoot = fgGetNode("/", false);
    JSBSim::FGPropertyManager* pm = _jsb->GetPropertyManager();

    _bindings.clear();

    // ------------------------------------------------------------
    // 3) Ensure JSBSim internal subsystem roots exist
    // ------------------------------------------------------------
    static const std::vector<std::string> jsbInternalRoots = {
        "/propulsion",
        "/engines",
        "/fcs",
        "/systems",
        "/gear",
        "/accelerations",
        "/aero",
        "/velocities"
    };

    for (const auto& root : jsbInternalRoots) {
        std::string full = "/fdm/jsbsim" + root;
        pm->GetNode(full.c_str(), /*create*/ true);
    }

    // ------------------------------------------------------------
    // 4) Create bindings FG <-> JSBSim
    // ------------------------------------------------------------
    for (const auto& p : mergedProps) {

        // Always create FG-side node first; FG may rely on its existence
        SGPropertyNode* fgNode = fgRoot->getNode(p.c_str(), true);

        // Skip binding for FG control inputs: JSBSim core already handles /controls/*
        if (p.rfind("/controls/", 0) == 0) {
            if (logAll) {
                SG_LOG(SG_GENERAL, SG_ALERT,
                       "Compatibility: SKIP /controls/ property " << p
                       << " (FG node created, no binding)");
            }
            // FG node exists now; we just don't create a JSBSim binding
            continue;
        }

        bool isJSBInternal =
            (p.compare(0, 12, "/propulsion/")    == 0 ||
             p.compare(0, 5,  "/fcs/")           == 0 ||
             p.compare(0, 9,  "/systems/")       == 0 ||
             p.compare(0, 9,  "/engines/")       == 0 ||
             p.compare(0, 6,  "/gear/")          == 0 ||
             p.compare(0, 15, "/accelerations/") == 0 ||
             p.compare(0, 6,  "/aero/")          == 0 ||
             p.compare(0, 12, "/velocities/")    == 0);

        std::string jsbPath = p;

        if (isJSBInternal) {
            jsbPath = "/fdm/jsbsim" + jsbPath;
            pm->GetNode(jsbPath.c_str(), /*create*/ true);
        }

        JSBSim::FGPropertyNode* jsbNode = pm->GetNode(jsbPath.c_str(), true);

        PropertyBinding::Direction dir;
        if (p.rfind("/systems/", 0) == 0) {
            dir = PropertyBinding::FG_TO_JSB;
        } else {
            dir = PropertyBinding::JSB_TO_FG;
        }

        PropertyBinding b;
        b.path    = p;
        b.fgNode  = fgNode;
        b.jsbNode = jsbNode;
        b.dir     = dir;
        _bindings.push_back(b);

        if (logAll) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "Compatibility: bound FG <-> JSBSim property " << p
                   << " dir=" << (dir == PropertyBinding::FG_TO_JSB ? "FG->JSB" : "JSB->FG"));
        }
    }

    if (!logAll) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Compatibility scan: created " << _bindings.size()
               << " JSBSim internal nodes and "
               << mergedProps.size() << " bindings");
    }

    SG_LOG(SG_GENERAL, SG_ALERT, "Compatibility scan complete");
    return true;
}

static bool isDotOrDotDot(const char* name)
{
    return (std::strcmp(name, ".") == 0) || (std::strcmp(name, "..") == 0);
}

void AddonJSBSimAnywhereEngine::collectAllXMLFiles(
        const SGPath& root,
        std::vector<SGPath>& out)
{
    struct stat st;
    if (::stat(root.str().c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
        return;

    DIR* d = ::opendir(root.str().c_str());
    if (!d)
        return;

    struct dirent* ent;
    while ((ent = ::readdir(d)) != nullptr) {
        if (isDotOrDotDot(ent->d_name))
            continue;

        SGPath p = root / ent->d_name;

        if (::stat(p.str().c_str(), &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            // FULL recursive descent
            collectAllXMLFiles(p, out);
        }
        else if (S_ISREG(st.st_mode)) {
            std::string ext = p.extension();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".xml" || ext == "xml") {
                out.push_back(p);
            }
        }
    }

    ::closedir(d);
}

void AddonJSBSimAnywhereEngine::collectXMLFilesInDirectory(
        const SGPath& dirPath,
        std::vector<SGPath>& out)
{
    struct stat st;
    if (::stat(dirPath.str().c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
        return;

    DIR* d = ::opendir(dirPath.str().c_str());
    if (!d)
        return;

    struct dirent* ent;
    while ((ent = ::readdir(d)) != nullptr) {
        if (isDotOrDotDot(ent->d_name))
            continue;

        SGPath p = dirPath / ent->d_name;

        if (::stat(p.str().c_str(), &st) != 0)
            continue;

        if (S_ISREG(st.st_mode)) {
            std::string ext = p.extension();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".xml" || ext == "xml") {
                out.push_back(p);
            }
        }
    }

    ::closedir(d);
}

void AddonJSBSimAnywhereEngine::init()
{
    SG_LOG(SG_GENERAL, SG_ALERT, "JSBSimAnywhereEngine::init() CALLED");

    configureFromProperties();
    loadCompatibilityConfig();

    // ------------------------------------------------------------
    // Common path variables
    // ------------------------------------------------------------
    SGPath rootDir;
    SGPath aircraftPath;
    SGPath enginePath;
    SGPath systemsPath;

    std::string modelName     = _fdmModel;
    std::string modelFileName = _fdmModel + ".xml";

    // ------------------------------------------------------------
    // BENCHMARK MODE (always internal)
    // ------------------------------------------------------------
    if (_mode == "benchmark") {

        SG_LOG(SG_GENERAL, SG_ALERT, "=== BENCHMARK MODE ===");

        rootDir      = SGPath(_addonPath) / "JSBSim";
        aircraftPath = rootDir / _fdmModel;
        enginePath   = aircraftPath / "engines";
        systemsPath  = aircraftPath / "systems";
    }

    // ------------------------------------------------------------
    // AIRCRAFT MODE (internal or external)
    // ------------------------------------------------------------
    else if (_mode == "aircraft") {

        SG_LOG(SG_GENERAL, SG_ALERT, "=== AIRCRAFT MODE ===");

        if (_fdmPath.empty()) {
            // Internal aircraft under addon
            rootDir      = SGPath(_addonPath) / "JSBSim";
            aircraftPath = rootDir / _fdmModel;
        } else {
            // External aircraft
            rootDir      = SGPath(_fdmPath);
            aircraftPath = rootDir;
        }

        enginePath  = aircraftPath / "Engines";
        systemsPath = aircraftPath / "Systems";
    }

    // ------------------------------------------------------------
    // Unknown mode
    // ------------------------------------------------------------
    else {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AddonJSBSimAnywhereEngine: unknown mode: " << _mode);
        return;
    }

    // ------------------------------------------------------------
    // Debug output
    // ------------------------------------------------------------
    if (_debug) {
        SG_LOG(SG_GENERAL, SG_ALERT, "=== JSBSim PATH DEBUG START ===");
        SG_LOG(SG_GENERAL, SG_ALERT, "addonPath     = " << _addonPath);
        SG_LOG(SG_GENERAL, SG_ALERT, "rootDir       = " << rootDir.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "aircraftPath  = " << aircraftPath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "modelFileName = " << modelFileName);
        SG_LOG(SG_GENERAL, SG_ALERT, "modelName     = '" << modelName << "'");
        SG_LOG(SG_GENERAL, SG_ALERT, "enginePath    = " << enginePath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "systemsPath   = " << systemsPath.str());
        SG_LOG(SG_GENERAL, SG_ALERT, "=== JSBSim PATH DEBUG END ===");
    }

    // ------------------------------------------------------------
    // Create JSBSim engine and configure search paths
    // ------------------------------------------------------------
    _jsb = std::make_unique<JSBSim::FGFDMExec>(nullptr, nullptr);
    _jsb->SetRootDir(rootDir.str());
    _jsb->SetAircraftPath(aircraftPath.str());
    _jsb->SetEnginePath(enginePath.str());
    _jsb->SetSystemsPath(systemsPath.str());

    // ------------------------------------------------------------
    // Compatibility scan (if enabled)
    // ------------------------------------------------------------
    if (_compatibilityEnabled) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSimAnywhere: running compatibility scan in "
               << aircraftPath.str());

        runCompatibilityScan(aircraftPath,
                             _compatibilityFullScan,
                             _compatibilityLogAll);
    }

    // ------------------------------------------------------------
    // Load the JSBSim model
    // ------------------------------------------------------------
    SG_LOG(SG_GENERAL, SG_ALERT,
           "DEBUG: LoadModel path = " << _jsb->GetAircraftPath());
    SG_LOG(SG_GENERAL, SG_ALERT,
           "DEBUG: modelName = " << modelName);
    SG_LOG(SG_GENERAL, SG_ALERT,
           "DEBUG: modelFileName = " << modelFileName);

    SG_LOG(SG_GENERAL, SG_ALERT,
           "Calling JSBSim::LoadModel() for model '" << modelName << "'");

    try {
        if (!_jsb->LoadModel(modelName,
                             enginePath.str(),
                             systemsPath.str(),
                             modelName,
                             false))   // ALWAYS false for FG's JSBSim
        {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "JSBSim: LoadModel() FAILED for " << aircraftPath.str());
            return;
        }
    } catch (const std::exception& e) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSimAnywhere: EXCEPTION in init(): " << e.what());
        return;
    } catch (...) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSBSimAnywhere: UNKNOWN EXCEPTION in init()");
        return;
    }

    // ------------------------------------------------------------
    // Create FG property nodes for JSBSim engine values
    // ------------------------------------------------------------
    SGPropertyNode_ptr propRoot = fgGetNode("propulsion", /*create*/ true);

    // num-engines
    _numEnginesNode = propRoot->getChild("num-engines", 0, /*create*/ true);

    // engine[0] subtree
    SGPropertyNode_ptr eng0Node = propRoot->getChild("engine", 0, /*create*/ true);

    _eng0_rpm            = eng0Node->getChild("rpm", 0, /*create*/ true);
    _eng0_starter_rpm    = eng0Node->getChild("starter-rpm", 0, /*create*/ true);
    _eng0_running        = eng0Node->getChild("running", 0, /*create*/ true);
    _eng0_throttle_norm  = eng0Node->getChild("throttle-cmd-norm", 0, /*create*/ true);
    _eng0_mixture_norm   = eng0Node->getChild("mixture-cmd-norm", 0, /*create*/ true);

    // propeller subtree
    SGPropertyNode_ptr propNode =
        eng0Node->getChild("propeller", 0, /*create*/ true);

    _eng0_prop_rpm = propNode->getChild("rpm", 0, /*create*/ true);

JSBSim::FGPropulsion* prop = _jsb->GetPropulsion();

if (!prop) {
SG_LOG(SG_GENERAL, SG_ALERT,
       "JSBSimAnywhere: FGPropulsion is null");
return;
}

int nEng = prop->GetNumEngines();
SG_LOG(SG_GENERAL, SG_ALERT,
   "JSBSimAnywhere: GetNumEngines() = " << nEng);

for (int i = 0; i < nEng; ++i) {
JSBSim::FGEngine* eng = prop->GetEngine(i);
if (eng) {
    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSBSimAnywhere: engine[" << i << "] name=" << eng->GetName());
} else {
    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSBSimAnywhere: engine[" << i << "] is null");
}
}

    SG_LOG(SG_GENERAL, SG_ALERT, "JSBSim: LoadModel() SUCCEEDED");

    JSBSim::FGPropertyManager* pm = _jsb->GetPropertyManager();

    // ------------------------------------------------------------
    // Probe propulsion existence in JSBSim internal tree
    // ------------------------------------------------------------
    auto* root = pm->GetNode("propulsion", false);
    auto* n1   = pm->GetNode("propulsion/num-engines", false);
    auto* n2   = pm->GetNode("propulsion/num_engines", false);

    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSB PROBE: propulsion root=" << (root ? "YES" : "NO"));
    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSB PROBE: propulsion/num-engines node=" << (n1 ? "YES" : "NO"));
    SG_LOG(SG_GENERAL, SG_ALERT,
           "JSB PROBE: propulsion/num_engines node=" << (n2 ? "YES" : "NO"));

    if (n1) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSB PROBE: propulsion/num-engines value=" << n1->getDoubleValue());
    }
    if (n2) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "JSB PROBE: propulsion/num_engines value=" << n2->getDoubleValue());
    }

    // ------------------------------------------------------------
    // Run initial conditions
    // ------------------------------------------------------------
    if (!_jsb->RunIC()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "JSBSim: RunIC() FAILED");
        return;
    }

    SG_LOG(SG_GENERAL, SG_INFO,
           "JSBSimAnywhereEngine: JSBSim addon FDM initialized successfully");

    // ------------------------------------------------------------
    // Initial property dump (JSBSim internal paths)
    // ------------------------------------------------------------
    auto dump = [&](const char* name) {
        JSBSim::FGPropertyNode* n = pm->GetNode(name, false);
        double v = n ? n->getDoubleValue() : -9999.0;
        SG_LOG(SG_GENERAL, SG_ALERT,
               std::string("JSB INIT PROP: ") + name + " = " + std::to_string(v));
    };

    dump("propulsion/num-engines");
    dump("propulsion/num_engines");
    dump("propulsion/engine[0]/rpm");
    dump("propulsion/engine[0]/starter-rpm");
    dump("propulsion/engine[0]/running");
    dump("propulsion/engine[0]/throttle-cmd-norm");
    dump("propulsion/engine[0]/mixture-cmd-norm");
    dump("propulsion/magnetos_cmd");
    dump("propulsion/engine[0]/propeller/rpm");

    // ------------------------------------------------------------
    // Addon control bindings (Nasal → FG → JSBSim)
    // ------------------------------------------------------------
    {
        // Throttle
        {
            PropertyBinding b;
            b.dir   = PropertyBinding::FG_TO_JSB;
            b.fgNode  = fgGetNode("/addon/controls/jsbsim/throttle", true);
            b.jsbNode = pm->GetNode("propulsion/engine[0]/throttle-cmd-norm", true);
            _bindings.push_back(b);
        }

        // Mixture
        {
            PropertyBinding b;
            b.dir   = PropertyBinding::FG_TO_JSB;
            b.fgNode  = fgGetNode("/addon/controls/jsbsim/mixture", true);
            b.jsbNode = pm->GetNode("propulsion/engine[0]/mixture-cmd-norm", true);
            _bindings.push_back(b);
        }

        // Starter
        {
            PropertyBinding b;
            b.dir   = PropertyBinding::FG_TO_JSB;
            b.fgNode  = fgGetNode("/addon/controls/jsbsim/starter", true);
            b.jsbNode = pm->GetNode("propulsion/engine[0]/starter_cmd", true);
            _bindings.push_back(b);
        }

        // Magnetos
        {
            PropertyBinding b;
            b.dir   = PropertyBinding::FG_TO_JSB;
            b.fgNode  = fgGetNode("/addon/controls/jsbsim/magnetos", true);
            b.jsbNode = pm->GetNode("propulsion/magnetos_cmd", true);
            _bindings.push_back(b);
        }
    }
}

void AddonJSBSimAnywhereEngine::update(double dt)
{
    if (!_jsb)
        return;

    JSBSim::FGPropertyManager* pm = _jsb->GetPropertyManager();

    // ------------------------------------------------------------
    // 1. FG → JSBSim (apply inputs BEFORE stepping)
    // ------------------------------------------------------------
    if (_compatibilityEnabled) {
        for (auto& b : _bindings) {
            if (b.dir == PropertyBinding::FG_TO_JSB) {
                double v = b.fgNode->getDoubleValue();
                b.jsbNode->setDoubleValue(v);
            }
        }
    }

    // ------------------------------------------------------------
    // HARD-WIRED TEST INPUTS (correct JSBSim internal paths)
    // ------------------------------------------------------------
    _jsb->SetPropertyValue("propulsion/engine[0]/throttle-cmd-norm", 0.2);
    _jsb->SetPropertyValue("propulsion/engine[0]/mixture-cmd-norm", 1.0);
    _jsb->SetPropertyValue("propulsion/magnetos_cmd", 3.0);
    _jsb->SetPropertyValue("propulsion/engine[0]/starter_cmd", 1.0);

    // ------------------------------------------------------------
    // 2. Run JSBSim step
    // ------------------------------------------------------------
    _jsb->Setdt(dt);
    _jsb->Run();

    // ------------------------------------------------------------
    // Publish JSBSim engine values into FG property tree
    // using JSBSim's own property manager
    // ------------------------------------------------------------
    JSBSim::FGPropertyManager* jpm = _jsb->GetPropertyManager();
    if (!jpm) {
        return;
    }

    // num-engines
    if (JSBSim::FGPropertyNode* nEngNode = jpm->GetNode("propulsion/num-engines")) {
        _numEnginesNode->setIntValue(static_cast<int>(nEngNode->getDoubleValue()));
    }

    // engine[0] rpm
    if (JSBSim::FGPropertyNode* rpmNode = jpm->GetNode("propulsion/engine[0]/rpm")) {
        _eng0_rpm->setDoubleValue(rpmNode->getDoubleValue());
    }

    // starter-rpm
    if (JSBSim::FGPropertyNode* sRpmNode = jpm->GetNode("propulsion/engine[0]/starter-rpm")) {
        _eng0_starter_rpm->setDoubleValue(sRpmNode->getDoubleValue());
    }

    // running
    if (JSBSim::FGPropertyNode* runNode = jpm->GetNode("propulsion/engine[0]/running")) {
        _eng0_running->setBoolValue(runNode->getBoolValue());
    }

    // throttle-cmd-norm
    if (JSBSim::FGPropertyNode* thrNode = jpm->GetNode("propulsion/engine[0]/throttle-cmd-norm")) {
        _eng0_throttle_norm->setDoubleValue(thrNode->getDoubleValue());
    }

    // mixture-cmd-norm
    if (JSBSim::FGPropertyNode* mixNode = jpm->GetNode("propulsion/engine[0]/mixture-cmd-norm")) {
        _eng0_mixture_norm->setDoubleValue(mixNode->getDoubleValue());
    }

    // propeller rpm
    if (JSBSim::FGPropertyNode* pRpmNode = jpm->GetNode("propulsion/engine[0]/propeller/rpm")) {
        _eng0_prop_rpm->setDoubleValue(pRpmNode->getDoubleValue());
    }

    // ------------------------------------------------------------
    // 2b. LOG JSBSim INTERNAL STATE (correct paths)
    // ------------------------------------------------------------
    auto dump = [&](const char* name) {
        JSBSim::FGPropertyNode* n = pm->GetNode(name, false);
        double v = n ? n->getDoubleValue() : -9999.0;
        SG_LOG(SG_GENERAL, SG_ALERT,
               std::string("JSB STEP PROP: ") + name + " = " + std::to_string(v));
    };

    dump("propulsion/num_engines");
    dump("propulsion/engine[0]/rpm");
    dump("propulsion/engine[0]/starter-rpm");
    dump("propulsion/engine[0]/running");
    dump("propulsion/engine[0]/propeller/rpm");
    dump("propulsion/engine[0]/torque");
    dump("propulsion/engine[0]/power");

    SG_LOG(SG_GENERAL, SG_ALERT,
        "JSBSimAnywhere TEST: thr=" << _jsb->GetPropertyValue("propulsion/engine[0]/throttle-cmd-norm")
        << " mix="    << _jsb->GetPropertyValue("propulsion/engine[0]/mixture-cmd-norm")
        << " mags="   << _jsb->GetPropertyValue("propulsion/magnetos_cmd")
        << " start_c="<< _jsb->GetPropertyValue("propulsion/engine[0]/starter_cmd"));

    // ------------------------------------------------------------
    // 3. JSBSim → FG (publish outputs AFTER stepping)
    // ------------------------------------------------------------
    if (_compatibilityEnabled) {
        for (auto& b : _bindings) {
            if (b.dir == PropertyBinding::JSB_TO_FG) {
                double v = b.jsbNode->getDoubleValue();
                b.fgNode->setDoubleValue(v);
            }
        }
    }

    // ------------------------------------------------------------
    // 4. Addon-specific publish (FG wrapper)
    // ------------------------------------------------------------
    for (const auto& p : _publishProps)
    {
        double val = _jsb->GetPropertyValue(p);
        fgSetDouble("/addon/fdm/jsbsim/" + p, val);
    }

    SG_LOG(SG_GENERAL, SG_DEBUG,
       "FG thr=" << fgGetDouble("/addon/controls/jsbsim/throttle")
       << " JSB thr=" << _jsb->GetPropertyValue("propulsion/engine[0]/throttle-cmd-norm"));
}

} // namespace addons
} // namespace flightgear
