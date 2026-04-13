#pragma once
#include <string>
#include <set>
#include <vector>

#include <simgear/xml/easyxml.hxx>

// PropertyTreeScanner:
// Scans XML files for property-like strings and classifies them as either
// JSBSim properties or FG-side properties depending on scan mode.

class PropertyTreeScanner : public XMLVisitor {
public:
    enum class Target {
        NONE,
        JSBSIM,
        FG
    };

    PropertyTreeScanner();

    // Scan a JSBSim FDM XML file
    void scanJSBSimFile(const std::string& path);

    // Scan FG-side XML (systems, autopilot, animations, nasal, etc.)
    void scanFGFile(const std::string& path);

    // Add FG-side properties manually (used by PropertyListScanner)
    void addFGProperty(const std::string& p);

    // Accessors
    const std::set<std::string>& getJSBSimProperties() const { return _jsbProperties; }
    const std::set<std::string>& getFGProperties() const { return _fgProperties; }

private:
    // Classification sets
    std::set<std::string> _jsbProperties;
    std::set<std::string> _fgProperties;
    std::string _currentElement;

    // Current scan target
    Target _currentTarget = Target::NONE;

    // XMLVisitor overrides
    void startElement(const char* name, const XMLAttributes& atts) override;
    void endElement(const char* name) override {}
    void data(const char* s, int length) override;

    // Helper to extract FG-style property-like strings from text nodes
    void extractProperties(const std::string& text);
};
