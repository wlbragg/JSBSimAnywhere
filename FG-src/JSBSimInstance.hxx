#pragma once

#include <string>
#include <memory>

class FGFDMExec;
class SGPropertyNode;

class JSBSimInstance {
public:
    JSBSimInstance(const std::string& id, const std::string& modelPath);
    ~JSBSimInstance();

    bool loadModel();
    void update(double dt);

    // FG <-> JSBSim property access
    double get(const std::string& path) const;
    void set(const std::string& path, double value);

    const std::string& getID() const { return _id; }

private:
    std::string _id;
    std::string _modelPath;

    std::unique_ptr<FGFDMExec> _fdm;

    void bindToFGProperties();
};
