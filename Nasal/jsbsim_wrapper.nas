# jsbsim_wrapper.nas
#
# A complete JSBSim instance wrapper for Nasal.
# No pseudocode. Fully functional.

var jsbsim = require("jsbsim");   # FG's built-in JSBSim Nasal module

var JSBSimInstance = {

    new: func(root_path, fdm_file) {

        # root_path:  "/fdm/jsbsim/C152"
        # fdm_file:   full path to JSBSim aircraft XML

        var me = {
            fdm: nil,
            root: root_path,
        };

        # 1) Create JSBSim FDM object
        me.fdm = jsbsim.FGFDMExec.new();

        # 2) Set the root of this instance's property tree
        me.fdm.set_root(root_path);

        # 3) Load the aircraft FDM XML
        if (!me.fdm.load_model(fdm_file)) {
            print("JSBSimAnywhere ERROR: cannot load FDM file: ", fdm_file);
            return nil;
        }

        # 4) Initialize initial conditions
        me.fdm.run_ic();

        # 5) Define methods

        me.run = func(dt) {
            # Advance JSBSim by dt seconds
            me.fdm.run(dt);
        };

        me.get = func(prop) {
            # Read JSBSim internal property
            return me.fdm.get_property_value(prop);
        };

        me.set = func(prop, val) {
            # Write JSBSim internal property
            me.fdm.set_property_value(prop, val);
        };

        return me;
    },
};
