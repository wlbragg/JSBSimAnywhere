// PropertyTreeBuilder.hxx
#pragma once
#include <string>
#include <set>

class PropertyTreeBuilder {
public:
    PropertyTreeBuilder(const std::string& addonRoot);

    // Build addon + FG compatibility trees
    void build(const std::set<std::string>& props);

private:
    std::string _addonRoot;

    // Normalize property paths
    std::string normalize(const std::string& path);

    // Create a node in FG property tree
    void createNode(const std::string& path);

    // Create addon-side node
    void createAddonNode(const std::string& path);

    // Create FG compatibility node
    void createFGNode(const std::string& path);
};
