#ifndef FG_PROPERTY_BINDING_HXX
#define FG_PROPERTY_BINDING_HXX

#include <simgear/props/props.hxx>

struct PropertyBinding
{
    enum Direction {
        FG_TO_JSB,
        JSB_TO_FG
    };

    Direction dir;

    // FG property node (FlightGear)
    SGPropertyNode_ptr fgNode;

    // JSBSim property node
    JSBSim::FGPropertyNode* jsbNode;
};

#endif
