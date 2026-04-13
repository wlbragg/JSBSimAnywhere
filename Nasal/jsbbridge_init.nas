# jsbsimanywhere.nas
#
# Complete multi-aircraft JSBSimAnywhere system.

var model = require("model");
var jsb_bridge_mod = require("jsb_bridge");

# -------------------------------
# 1. AIRCRAFT REGISTRY
# -------------------------------

var JSB_AIRCRAFT = {

    C152: {
        id: "C152",
        label: "Cessna 152",

        # JSBSim internal tree root for this instance
        jsb_root: "/fdm/jsbsim/C152",

        # Addon-visible tree for animations
        addon_root: "/addon/fdm/jsbsim/C152",

        # Model files
        model_xml: "[addon=org.flightgear.addons.jsbsimanywhere]JSBSim/TestPlanes/Models/c152.xml",
        model_ac:  "[addon=org.flightgear.addons.jsbsimanywhere]JSBSim/TestPlanes/Models/c152.ac",

        # Runtime fields
        jsb: nil,
        bridge: nil,
        model_node: nil,
    },

    # Add more aircraft here...
};

# -------------------------------
# 2. INITIALIZE AIRCRAFT
# -------------------------------

var jsb_init_aircraft = func(id) {

    var a = JSB_AIRCRAFT[id];
    if (a == nil) return;

    # 1) Create JSBSim instance (your wrapper)
    # Replace this with your actual JSBSimAnywhere constructor.
    a.jsb = jsbsim_new_instance(a.jsb_root);

    # 2) Initialize bridge
    jsb_init_bridge(id);

    # 3) Spawn model
    jsb_spawn_model(id);
};

# -------------------------------
# 3. INITIALIZE PROPERTY BRIDGE
# -------------------------------

var jsb_init_bridge = func(id) {

    var a = JSB_AIRCRAFT[id];
    if (a == nil) return;

    a.bridge = jsb_bridge_mod.JSBBridge.new(a.jsb_root, a.addon_root);

    var RAD2DEG = 57.29577951308232;

    # ENGINE
    a.bridge.add_map("propulsion/engine[0]/rpm",
                     "propulsion/engine[0]/rpm");

    a.bridge.add_map("propulsion/engine[0]/thrust-lbs",
                     "propulsion/engine[0]/thrust-lbs");

    # CONTROL SURFACES
    a.bridge.add_map("fcs/aileron-pos-rad",
                     "fcs/aileron-pos-deg",
                     RAD2DEG);

    a.bridge.add_map("fcs/elevator-pos-rad",
                     "fcs/elevator-pos-deg",
                     RAD2DEG);

    a.bridge.add_map("fcs/rudder-pos-rad",
                     "fcs/rudder-pos-deg",
                     RAD2DEG);

    # GEAR
    a.bridge.add_map("gear/gear-pos-norm",
                     "gear/gear-pos-norm");
};

# -------------------------------
# 4. SPAWN MODEL
# -------------------------------

var jsb_spawn_model = func(id) {

    var a = JSB_AIRCRAFT[id];
    if (a == nil) return nil;

    var ac_pos = geo.aircraft_position();
    var hdg   = getprop("/orientation/heading-deg");

    var p = geo.Coord.new(ac_pos);
    p.apply_course_distance(hdg, 2 * M2NM);

    var lat   = p.lat();
    var lon   = p.lon();
    var alt_m = p.alt();
    var alt_ft = alt_m * M2FT;

    var node = nil;

    if (a.model_xml != nil) {
        node = model.load(a.model_xml, lat, lon, alt_ft, hdg, 0, 0);
    }

    if (node == nil and a.model_ac != nil) {
        node = model.load(a.model_ac, lat, lon, alt_ft, hdg, 0, 0);
    }

    if (node == nil) {
        print("JSBSimAnywhere: FAILED to spawn model for ", id);
        return nil;
    }

    node.getNode("name", 1).setValue("JSBSimAnywhere-" ~ id);
    a.model_node = node;

    return node;
};

# -------------------------------
# 5. PER-FRAME STEP
# -------------------------------

var jsb_step = func(id, dt) {

    var a = JSB_AIRCRAFT[id];
    if (a == nil) return;

    # 1) Advance JSBSim instance
    if (a.jsb != nil) {
        a.jsb.run(dt);
    }

    # 2) Update bridge (push JSBSim → FG properties)
    if (a.bridge != nil) {
        a.bridge.update();
    }
};

# -------------------------------
# 6. GLOBAL UPDATE LOOP
# -------------------------------

var jsb_update_all = func {

    var dt = getprop("/sim/dt");

    foreach (var id; keys(JSB_AIRCRAFT)) {
        jsb_step(id, dt);
    }
};

# Start update loop when FDM is initialized
setlistener("/sim/signals/fdm-initialized", func {
    settimer(jsb_update_all, 0, true);
});

jsb_init_aircraft("C152");
