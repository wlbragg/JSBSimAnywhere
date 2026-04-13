include(FlightGearComponent)

# Add JSBSim include directory so <JSBSim/FGFDMExec.h> resolves
include_directories(${SIMGEAR_INCLUDE_DIRS})

set(SOURCES
    Addon.cxx
    AddonManager.cxx
    AddonMetadataParser.cxx
    AddonResourceProvider.cxx
    AddonVersion.cxx
    contacts.cxx
    exceptions.cxx
    FDMManager.cxx
    AddonEngineFactory.cxx
    PropertyTreeScanner.cxx
    PropertyTreeBuilder.cxx
    SetFileParser.cxx
    PropertyListScanner.cxx
    AddonJSBSimAnywhereEngine.cxx
    #JSBSimInstance.cxx
    #JSBSimEngine.cxx
)

set(HEADERS
    addon_fwd.hxx
    Addon.hxx
    AddonManager.hxx
    AddonMetadataParser.hxx
    AddonResourceProvider.hxx
    AddonVersion.hxx
    contacts.hxx
    exceptions.hxx
    pointer_traits.hxx
    FDMManager.hxx
    AddonEngineFactory.hxx
    PropertyTreeScanner.hxx
    PropertyTreeBuilder.hxx
    SetFileParser.hxx
    PropertyListScanner.hxx
    AddonJSBSimAnywhereEngine.hxx
    #JSBSimInstance.cxx
    #JSBSimEngine.cxx
)

flightgear_component(AddonManagement "${SOURCES}" "${HEADERS}")
