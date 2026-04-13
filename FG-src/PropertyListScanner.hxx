#pragma once
#include <string>
#include <vector>
#include <simgear/xml/easyxml.hxx>

class PropertyListScanner : public XMLVisitor {
public:
    PropertyListScanner();

    void scan(const std::string& setFile);

    const std::vector<std::string>& getProperties() const { return _properties; }

private:
    std::vector<std::string> _properties;
    std::vector<std::string> _stack;
    bool _inPropertyList = false;

    void startElement(const char* name, const XMLAttributes& atts) override;
    void endElement(const char* name) override;
    void data(const char* s, int length) override;
};
