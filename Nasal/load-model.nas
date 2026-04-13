# JSBSimAnywhere minimal model loader (MissionGenerator style)

var ADDON_BASE_PATH = getprop("/addons/jsbsimanywhere/base-path");

if (ADDON_BASE_PATH == nil) {
    print("JSBSimAnywhere ERROR: ADDON_BASE_PATH not set");
} else {
    print("Base path =" ~ ADDON_BASE_PATH);
}

var jsb_models = { models: [] };

var unload_jsb_models = func() {
    var m = pop(jsb_models.models);
    while (m != nil) {
        m.remove();
        m = pop(jsb_models.models);
    }
};

var spawn_jsbsim_model = func() {

    print("JSBSimAnywhere: spawn_jsbsim_model() called (minimal)");

    unload_jsb_models();

    # Position: use current aircraft, small forward offset
    var ac_pos = geo.aircraft_position();
    var hdg   = getprop("/orientation/heading-deg");

    var offset_m = 2;
    var p = geo.Coord.new(ac_pos);
    p.apply_course_distance(hdg, offset_m * M2NM);

    var lat = p.lat();
    var lon = p.lon();
    var alt_m = p.alt()-1;   # meters

    print("JSBSimAnywhere: placing at lat=", lat, " lon=", lon, " alt_m=", alt_m);

    # Model XML path
    var xml_path = "[addon=org.flightgear.addons.JSBSimAnywhere]JSBSim/TestPlane/Models/TestPlane.xml";
    print("JSBSimAnywhere: model XML = ", xml_path);

    # Create /models/jsbsimanywhere/model[n]
    var root = props.globals.getNode("/models/jsbsimanywhere", 1);
    var model = nil;

    for (var i = 0; 1; i += 1) {
        if (root.getChild("model", i, 0) == nil) {
            model = root.getChild("model", i, 1);
            break;
        }
    }

    if (model == nil) {
        print("JSBSimAnywhere ERROR: could not allocate /models/jsbsimanywhere/model[]");
        return;
    }

    model.getNode("name", 1).setValue("JSBSimAnywhere-TestPlane");
    model.getNode("path", 1).setValue(xml_path);
    model.getNode("latitude-deg", 1).setDoubleValue(lat);
    model.getNode("longitude-deg", 1).setDoubleValue(lon);
    model.getNode("elevation-ft", 1).setDoubleValue(alt_m * M2FT);
    model.getNode("heading-deg", 1).setDoubleValue(hdg);
    model.getNode("pitch-deg", 1).setDoubleValue(0);
    model.getNode("roll-deg", 1).setDoubleValue(0);
    model.getNode("load", 1).remove();

    append(jsb_models.models, model);

    print("JSBSimAnywhere: minimal model spawned at ", model.getPath());

};

