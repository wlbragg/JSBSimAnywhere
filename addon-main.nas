#JSBSimAnywhere.
#Written and developed by Wayne Bragg (wlbragg(at)cox.net)
#Copyright (C) 2021 Wayne Bragg (wlbragg(at)cox.net)
#addon-main.nas
#Version 1.0.0 02/06/2026
#This file is licensed under the GPL license version 2 or later.


# JSBSimAnywhere addon main Nasal entry point
# Mirrors the RampMarshall addon structure exactly

var ADDON_BASE_PATH = 0;
var ADDON_NAMESPACE = 0;

var main = func(addon) {

    print("Nasal JSBSimAnywhere addon initialized from path ", addon.basePath);
    logprint(LOG_INFO, "JSBSimAnywhere ", addon.basePath);

    #
    # Store addon path + namespace exactly like RampMarshall
    #
    ADDON_BASE_PATH = addon.basePath;
    ADDON_NAMESPACE = "__addon[" ~ addon.id ~ "]__";

    #
    # Make the base path available to load-model.nas
    # (because load-model.nas runs in its own namespace)
    #
    setprop("/addons/jsbsimanywhere/base-path", ADDON_BASE_PATH);

    #
    # Initialization listener — same pattern as RampMarshall
    #
    setlistener("/sim/signals/fdm-initialized", func {

        #
        # Load our aircraft visualization script(s)
        # (kept EXACTLY as you requested)
        #
        foreach(var script; ['Nasal/load-model.nas','Nasal/model-loaders.nas']) {
            var fname = addon.basePath ~ "/" ~ script;
            print("Nasal file loaded " ~ fname);
            io.load_nasal(fname, "JSBSimAnywhere");
        }

        #
        # After loading the script, call its spawn function
        # (this mirrors RampMarshall calling main_loop.init())
        #
        if (JSBSimAnywhere.place_animated_model != nil) {
            # Position: use current aircraft, small forward offset
            var ac_pos = geo.aircraft_position();
            var hdg   = getprop("/orientation/heading-deg");

            var offset_m = 2;
            var p = geo.Coord.new(ac_pos);
            p.apply_course_distance(hdg, offset_m * M2NM);

            var lat = p.lat();
            var lon = p.lon();
            var alt_m = p.alt();   # meters

            JSBSimAnywhere.spawn_jsbsim_model();
            #JSBSimAnywhere.place_animated_model(addon.basePath ~ "/JSBSim/TestPlane/Models/boat.xml", lat, lon, alt_m-.5, rand()*360, 0, rand()*90, "Plane");
            #JSBSimAnywhere.StaticModel.new("Plane", addon.basePath ~ "/JSBSim/TestPlane/Models/boat.xml");
            #print("JSBSimAnywhere: placing at lat=", lat, " lon=", lon, " alt_m=", alt_m);
            #print("JSBSimAnywhere: placing at lat=", ac_pos.lat(), " lon=", ac_pos.lon(), " alt_m=", ac_pos.alt());
        } else {
            print("JSBSimAnywhere: ERROR — spawn_jsbsim_model() not found in loaded script");
        }
        setprop("/addon/controls/jsbsim/throttle", 0.2);
        setprop("/addon/controls/jsbsim/mixture", 1.0);
        setprop("/addon/controls/jsbsim/magnetos", 3);
        setprop("/addon/controls/jsbsim/starter", 1);
    }, 0, 1);
}


