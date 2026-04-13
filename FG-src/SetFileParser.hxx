#pragma once
#include <string>
#include <vector>
#include <set>

class SetFileParser {
public:
    SetFileParser(const std::string& rootDir);

    // Entry point: parse the -set.xml file
    void parse(const std::string& setFile);

    // All discovered system/autopilot/animation/nasal files
    const std::set<std::string>& getAllFiles() const { return _allFiles; }

private:
    std::string _rootDir;

    // Track visited files to avoid infinite recursion
    std::set<std::string> _visited;

    // All discovered files (systems, autopilot, animation, nasal)
    std::set<std::string> _allFiles;

    // Depth-first recursive scan
    void scanFileRecursive(const std::string& file);

    // Resolve relative paths
    std::string resolve(const std::string& relative);

    // XML visitor
    class Visitor;
};
