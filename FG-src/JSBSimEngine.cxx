#include "JSBSimEngine.hxx"
#include "JSBSimInstance.hxx"

JSBSimEngine& JSBSimEngine::instance()
{
    static JSBSimEngine inst;
    return inst;
}

std::string JSBSimEngine::generateID()
{
    char buf[16];
    snprintf(buf, sizeof(buf), "fdm-%03d", _autoCounter++);
    return std::string(buf);
}

std::string JSBSimEngine::createInstance(const std::string& modelPath,
                                         const std::string& name)
{
    std::string id = name.empty() ? generateID() : name;

    // Prevent collisions
    if (_instances.count(id) > 0)
        id = generateID();

    auto inst = std::make_unique<JSBSimInstance>(id, modelPath);

    if (!inst->loadModel()) {
        return ""; // failed
    }

    _instances[id] = std::move(inst);
    return id;
}

void JSBSimEngine::destroyInstance(const std::string& id)
{
    _instances.erase(id);
}

JSBSimInstance* JSBSimEngine::get(const std::string& id)
{
    auto it = _instances.find(id);
    return (it != _instances.end()) ? it->second.get() : nullptr;
}

void JSBSimEngine::update(double dt)
{
    for (auto& kv : _instances)
        kv.second->update(dt);
}
