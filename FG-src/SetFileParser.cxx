#include "SetFileParser.hxx"
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

class SetFileParser::Visitor : public XMLVisitor {
public:
    Visitor(SetFileParser* parser, const std::string& currentFile)
        : _parser(parser), _currentFile(currentFile)
    {}

    void startElement(const char* name, const XMLAttributes& atts) override {
        _currentTag = name;

        // Track blocks
        if (_currentTag == "system")     _inSystem = true;
        if (_currentTag == "systems")    _inSystems = true;
        if (_currentTag == "property-rule") _inPropertyRule = true;
        if (_currentTag == "autopilot")  _inAutopilot = true;
        if (_currentTag == "Animation")  _inAnimation = true;
        if (_currentTag == "Nasal")      _inNasal = true;

        // Pattern: <system file="fuel"/>
        if (_inSystem && atts.hasAttribute("file")) {
            std::string rel = atts.getValue("file");
            std::string full = _parser->resolve(rel);
            _parser->scanFileRecursive(full);
        }
    }

    void endElement(const char* name) override {
        std::string tag(name);

        if (tag == "system")       _inSystem = false;
        if (tag == "systems")      _inSystems = false;
        if (tag == "property-rule") _inPropertyRule = false;
        if (tag == "autopilot")    _inAutopilot = false;
        if (tag == "Animation")    _inAnimation = false;
        if (tag == "Nasal")        _inNasal = false;

        _currentTag.clear();
    }

    void data(const char* s, int len) override {
        std::string text(s, len);

        // Trim whitespace
        if (text.find_first_not_of(" \t\r\n") == std::string::npos)
            return;

        // Pattern: <systems><path>Systems/systems.xml</path></systems>
        if (_inSystems && _currentTag == "path") {
            _parser->scanFileRecursive(_parser->resolve(text));
        }

        // Pattern: <property-rule><path>Systems/engine.xml</path></property-rule>
        if (_inPropertyRule && _currentTag == "path") {
            _parser->scanFileRecursive(_parser->resolve(text));
        }

        // Pattern: <autopilot><path>Systems/glass-rain.xml</path></autopilot>
        if (_inAutopilot && _currentTag == "path") {
            _parser->scanFileRecursive(_parser->resolve(text));
        }

        // Pattern: <Animation><path>Models/c152.xml</path></Animation>
        if (_inAnimation && _currentTag == "path") {
            _parser->scanFileRecursive(_parser->resolve(text));
        }

        // Pattern: <Nasal><script>c152.nas</script></Nasal>
        if (_inNasal && _currentTag == "script") {
            _parser->scanFileRecursive(_parser->resolve(text));
        }

        // Pattern: <include>fuel.xml</include> inside Systems XML
        if (_currentTag == "include") {
            _parser->scanFileRecursive(_parser->resolve(text));
        }
    }

private:
    SetFileParser* _parser;
    std::string _currentFile;
    std::string _currentTag;

    bool _inSystem = false;
    bool _inSystems = false;
    bool _inPropertyRule = false;
    bool _inAutopilot = false;
    bool _inAnimation = false;
    bool _inNasal = false;
};

SetFileParser::SetFileParser(const std::string& rootDir)
    : _rootDir(rootDir)
{
}

std::string SetFileParser::resolve(const std::string& relative)
{
    // Trim whitespace
    std::string rel = relative;
    while (!rel.empty() && isspace(rel.front())) rel.erase(rel.begin());
    while (!rel.empty() && isspace(rel.back())) rel.pop_back();

    // Case 1: Absolute path
    if (!rel.empty() && rel[0] == '/') {
        return rel;
    }

    // Case 2: Aircraft/<name>/... → strip prefix
    // Example: Aircraft/c152/Systems/fuel.xml
    if (rel.rfind("Aircraft/", 0) == 0) {
        // Remove "Aircraft/<aircraft-name>/" prefix
        size_t firstSlash = rel.find('/');
        size_t secondSlash = rel.find('/', firstSlash + 1);
        if (secondSlash != std::string::npos) {
            std::string stripped = rel.substr(secondSlash + 1);
            SGPath full = SGPath(_rootDir) / stripped;
            return full.str();
        }
    }

    // Case 3: Normal relative path
    SGPath full = SGPath(_rootDir) / rel;
    return full.str();
}

void SetFileParser::scanFileRecursive(const std::string& file)
{
    // Avoid infinite loops
    if (_visited.count(file))
        return;

    _visited.insert(file);
    _allFiles.insert(file);

    SG_LOG(SG_GENERAL, SG_ALERT, "SetFileParser: scanning file: " << file);

    try {
        Visitor visitor(this, file);
        readXML(SGPath(file), visitor);
    }
    catch (const sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_ALERT, "SetFileParser: failed to read " << file << ": " << e.getMessage());
    }
}

void SetFileParser::parse(const std::string& setFile)
{
    scanFileRecursive(setFile);
}
