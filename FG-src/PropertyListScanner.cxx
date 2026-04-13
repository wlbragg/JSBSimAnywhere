#include "PropertyListScanner.hxx"
#include <simgear/debug/logstream.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>   // <-- REQUIRED for SGPath
#include <cstring>                    // <-- REQUIRED for strcmp

PropertyListScanner::PropertyListScanner()
{
}

void PropertyListScanner::scan(const std::string& setFile)
{
    SG_LOG(SG_GENERAL, SG_ALERT, "PropertyListScanner: scanning inline properties in " << setFile);

    try {
        readXML(SGPath(setFile), *this);
    }
    catch (const sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_ALERT, "PropertyListScanner failed: " << e.getMessage());
    }
}

void PropertyListScanner::startElement(const char* name, const XMLAttributes& atts)
{
    // Detect entering <PropertyList>
    if (!_inPropertyList) {
        if (strcmp(name, "PropertyList") == 0) {
            _inPropertyList = true;
        }
        return;
    }

    // Build segment
    std::string segment = name;

    // Handle array index: <tank n="1">
    if (atts.hasAttribute("n")) {
        segment += "[" + std::string(atts.getValue("n")) + "]";
    }

    _stack.push_back(segment);
}

void PropertyListScanner::endElement(const char* name)
{
    if (!_inPropertyList)
        return;

    if (strcmp(name, "PropertyList") == 0) {
        _inPropertyList = false;
        _stack.clear();
        return;
    }

    if (!_stack.empty())
        _stack.pop_back();
}

void PropertyListScanner::data(const char* s, int length)
{
    if (!_inPropertyList)
        return;

    // Ignore whitespace-only text
    std::string text(s, length);
    for (char c : text) {
        if (!isspace(c)) {
            // Build full path
            std::string path;
            for (size_t i = 0; i < _stack.size(); ++i) {
                if (i > 0) path += "/";
                path += _stack[i];
            }

            _properties.push_back(path);
            SG_LOG(SG_GENERAL, SG_ALERT, "PropertyListScanner: found inline property: " << path);
            break;
        }
    }
}
