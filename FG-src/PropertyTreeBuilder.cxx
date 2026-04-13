#include "PropertyTreeBuilder.hxx"
#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>

PropertyTreeBuilder::PropertyTreeBuilder(const std::string& addonRoot)
    : _addonRoot(addonRoot)
{
}

std::string PropertyTreeBuilder::normalize(const std::string& path)
{
    std::string p = path;

    // Remove leading slashes
    while (!p.empty() && p[0] == '/')
        p.erase(0, 1);

    // Remove trailing spaces
    while (!p.empty() && isspace(p.back()))
        p.pop_back();

    return p;
}

void PropertyTreeBuilder::createNode(const std::string& path)
{
    if (path.empty())
        return;

    fgGetNode(path.c_str(), true);
}

void PropertyTreeBuilder::createAddonNode(const std::string& path)
{
    std::string full = _addonRoot + "/" + path;
    createNode(full);
}

void PropertyTreeBuilder::createFGNode(const std::string& path)
{
    // FG compatibility tree uses the same path
    createNode(path);
}

void PropertyTreeBuilder::build(const std::set<std::string>& props)
{
    SG_LOG(SG_GENERAL, SG_ALERT, "=== PropertyTreeBuilder: Creating nodes ===");

    for (const auto& raw : props) {
        std::string path = normalize(raw);

        if (path.empty())
            continue;

        // Addon-side node ONLY for now
        //createAddonNode(path);

        // TEMP: disable FG compatibility nodes to isolate crash
        // createFGNode(path);

        SG_LOG(SG_GENERAL, SG_ALERT, "Created nodes for: " << path);
    }

    SG_LOG(SG_GENERAL, SG_ALERT, "=== PropertyTreeBuilder: Done ===");
}

