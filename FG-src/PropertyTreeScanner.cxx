#include "PropertyTreeScanner.hxx"
#include <simgear/debug/logstream.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>
#include <cstring>

PropertyTreeScanner::PropertyTreeScanner()
{
}

void PropertyTreeScanner::scanJSBSimFile(const std::string& path)
{
    _currentTarget = Target::JSBSIM;

    SG_LOG(SG_GENERAL, SG_DEBUG,
           "PropertyTreeScanner: scanning JSBSim file: " << path);

    try {
        readXML(SGPath(path), *this);
    }
    catch (const sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_DEBUG,
               "PropertyTreeScanner JSBSim scan failed: " << e.getMessage());
    }

    _currentTarget = Target::NONE;
}

void PropertyTreeScanner::scanFGFile(const std::string& path)
{
    _currentTarget = Target::FG;

    SG_LOG(SG_GENERAL, SG_DEBUG, "PropertyTreeScanner: scanning FG file: " << path);

    try {
        readXML(SGPath(path), *this);
    }
    catch (const sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "PropertyTreeScanner FG scan failed: " << e.getMessage());
    }

    _currentTarget = Target::NONE;
}

void PropertyTreeScanner::startElement(const char* name, const XMLAttributes& atts)
{
    _currentElement = name;
}

void PropertyTreeScanner::data(const char* s, int length)
{
    std::string raw(s, length);

    SG_LOG(SG_GENERAL, SG_DEBUG,
           "DATA: target=" << (int)_currentTarget
           << " elem=" << _currentElement
           << " raw='" << raw << "'");

    if (_currentTarget == Target::NONE)
        return;

    // Trim whitespace
    auto trim = [](std::string& t) {
        size_t start = t.find_first_not_of(" \t\r\n");
        size_t end   = t.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) {
            t.clear();
        } else {
            t = t.substr(start, end - start + 1);
        }
    };

    std::string text = raw;
    trim(text);

    if (text.empty())
        return;

    // ------------------------------------------------------------
    // JSBSim mode: ONLY extract <property> elements
    // ------------------------------------------------------------
    if (_currentTarget == Target::JSBSIM) {

        if (_currentElement == "property") {

            if (text[0] != '/')
                text = "/" + text;

            _jsbProperties.insert(text);

            SG_LOG(SG_GENERAL, SG_DEBUG,
                   "PropertyTreeScanner(JSBSim): property: " << text);
        }

        return;
    }

    // ------------------------------------------------------------
    // FG mode: extract FG-style property tokens
    // ------------------------------------------------------------
    if (_currentTarget == Target::FG) {
        extractProperties(text);
    }
}

void PropertyTreeScanner::extractProperties(const std::string& text)
{
    std::istringstream iss(text);
    std::string token;

    bool foundProperty = false;

    static const std::set<std::string> ops = {"eq", "le", "lt", "ge", "gt"};

    while (iss >> token) {

        if (foundProperty)
            continue;

        if (ops.count(token))
            continue;

        while (!token.empty() && (token.back() == ';' || token.back() == ',')) {
            token.pop_back();
        }

        if (token.empty())
            continue;

        if (token.find('/') == std::string::npos)
            continue;

        if (!(token[0] == '/' || std::isalpha(token[0])))
            continue;

        if (token.find('%') != std::string::npos)
            continue;

        if (token.front() == '[' && token.back() == ']')
            continue;

        bool digitThenLetter = false;
        for (size_t i = 0; i + 1 < token.size(); ++i) {
            if (std::isdigit((unsigned char)token[i]) &&
                std::isalpha((unsigned char)token[i+1])) {
                digitThenLetter = true;
                break;
            }
        }
        if (digitThenLetter)
            continue;

        if (token.find('(') != std::string::npos ||
            token.find(')') != std::string::npos ||
            token.find(' ') != std::string::npos)
            continue;

        if (token.find("kg") != std::string::npos ||
            token.find("ft") != std::string::npos ||
            token.find("deg") != std::string::npos ||
            token.find("m/") != std::string::npos)
            continue;

        if (token[0] != '/')
            token = "/" + token;

        bool hasDot = (token.find('.') != std::string::npos);
        if (hasDot) {
            bool numeric = true;
            for (char c : token) {
                if (!(std::isdigit((unsigned char)c) || c == '.' || c == '/' || c == '[' || c == ']')) {
                    numeric = false;
                    break;
                }
            }
            if (!numeric)
                continue;
        }

        _fgProperties.insert(token);

        SG_LOG(SG_GENERAL, SG_DEBUG,
               "PropertyTreeScanner(FG): property: " << token);

        foundProperty = true;
    }
}

