#pragma once

#include <string>
#include <map>
#include <memory>

class JSBSimInstance;

class JSBSimEngine {
public:
    static JSBSimEngine& instance();

    // Create instance (name optional)
    std::string createInstance(const std::string& modelPath,
                               const std::string& name = "");

    // Destroy instance
    void destroyInstance(const std::string& id);

    // Access instance
    JSBSimInstance* get(const std::string& id);

    // Update all instances
    void update(double dt);

private:
    JSBSimEngine() = default;

    std::string generateID();

    std::map<std::string, std::unique_ptr<JSBSimInstance>> _instances;
    int _autoCounter = 1;
};
