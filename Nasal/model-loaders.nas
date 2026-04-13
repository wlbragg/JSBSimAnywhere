var ADDON_BASE_PATH = getprop("/addons/jsbsimanywhere/base-path");

if (ADDON_BASE_PATH == nil) {
    print("JSBSimAnywhere ERROR: ADDON_BASE_PATH not set");
}

#################################################
# used for the incident models
var Model = {
    new : func(name, path, pos, hdg, pitch, roll) {
        var m = { parents: [Model] };
        m.name = name;
        m.pos = pos;
        m.path = path;
        m.selected = 1;
        m.visible = 1;

        var models = props.globals.getNode("/models/incident", 1);
        for (var i = 0; 1; i += 1) {
            if (models.getChild("model", i, 0) == nil) {
                m.node = models.getChild("model", i, 1);
                break;
            }
        }

        m.node.getNode( "name", 1).setValue( name );
        m.node.getNode( "path", 1).setValue( path );
        m.node.getNode( "latitude-deg", 1 ).setValue( pos.lat() );
        m.node.getNode( "longitude-deg", 1 ).setValue( pos.lon() );
        #m.node.getNode( "elevation-ft", 1 ).setValue( pos.alt()-2 * M2FT );
		m.node.getNode( "elevation-ft", 1 ).setValue( pos.alt() * 3.2808 );
        m.node.getNode( "heading-deg", 1 ).setValue( hdg );
        m.node.getNode( "pitch-deg", 1 ).setValue( pitch );
        m.node.getNode( "roll-deg", 1 ).setValue( roll );
        m.node.getNode( "load", 1).remove();
        return m;
    },
    remove : func {
        me.node.remove();
    }
};

###############################################################################
# Add static objects to scene
# used for smoke, need to use one of the other existing functions
var StaticModel = {
    new: func (name, file) {
        var m = {
            parents: [StaticModel],
            model: nil,
            model_file: file,
            object_name: name
        };

        setlistener("/sim/" ~ name ~ "/enable", func (node) {
            if (node.getBoolValue()) {
                m.add();
            }
            else {
                m.remove();
            }
        });

        return m;
    },

    add: func {
        var manager = props.globals.getNode("/models", 1);
        var i = 0;
        for (; 1; i += 1) {
            if (manager.getChild("model", i, 0) == nil) {
                break;
            }
        }
        var position = geo.aircraft_position().set_alt(getprop("/position/ground-elev-m"));
        me.model = geo.put_model(me.model_file, position, getprop("/orientation/heading-deg"));
    },

    remove: func {
        if (me.model != nil) {
            me.model.remove();
            me.model = nil;
        }
    }
};

################################################################################
# used to access all models as cargo, experimental
var place_model = func(number, position, desc, path, stack, drop, weight, height, harness, lat, lon, alt, heading, pitch, roll, ai) {

    var m = props.globals.getNode("models", 1);
    for (var i = 0; 1; i += 1)
        if (m.getChild("model", i, 0) == nil)
            break;
    var model = m.getChild("model", i, 1);

    setprop("/models/cargo/cargo["~position~"]/latitude-deg", lat);
    setprop("/models/cargo/cargo["~position~"]/longitude-deg", lon);
    setprop("/models/cargo/cargo["~position~"]/elevation-ft", alt);
    setprop("/models/cargo/cargo["~position~"]/heading-deg", heading);
    setprop("/models/cargo/cargo["~position~"]/pitch-deg", pitch);
    setprop("/models/cargo/cargo["~position~"]/roll-deg", roll);
    setprop("/models/cargo/cargo["~position~"]/callsign", "cargo"~number);
    setprop("/models/cargo/cargo["~position~"]/description", desc);
    setprop("/models/cargo/cargo["~position~"]/weight", weight);
    setprop("/models/cargo/cargo["~position~"]/height", height);
    setprop("/models/cargo/cargo["~position~"]/harness", harness);
    setprop("/models/cargo/cargo["~position~"]/stack", stack);
    setprop("/models/cargo/cargo["~position~"]/drop", drop);
    setprop("/models/cargo/cargo["~position~"]/ai", ai);

    var cargomodel = props.globals.getNode("/models/cargo/cargo["~position~"]", 1);
    var latN = cargomodel.getNode("latitude-deg",1);
    var lonN = cargomodel.getNode("longitude-deg",1);
    var altN = cargomodel.getNode("elevation-ft",1);
    var headN = cargomodel.getNode("heading-deg",1);
    var pitchN = cargomodel.getNode("pitch-deg",1);
    var rollN = cargomodel.getNode("roll-deg",1);
    var callsignN = cargomodel.getNode("callsign",1);
    var descriptionN = cargomodel.getNode("description",1);
    var weightN = cargomodel.getNode("weight",1);
    var heightN = cargomodel.getNode("height",1);
    var harnessN = cargomodel.getNode("harness",1);
    var stackN = cargomodel.getNode("stack",1);
    var dropN = cargomodel.getNode("drop",1);
    var aiN = cargomodel.getNode("ai",1);

    #model.getNode("path", 1).setValue(path~"cargo"~number~".xml");
    model.getNode("path", 1).setValue(path);
    model.getNode("latitude-deg-prop", 1).setValue(latN.getPath());
    model.getNode("longitude-deg-prop", 1).setValue(lonN.getPath());
    model.getNode("elevation-ft-prop", 1).setValue(altN.getPath());
    model.getNode("heading-deg-prop", 1).setValue(headN.getPath());
    model.getNode("pitch-deg-prop", 1).setValue(pitchN.getPath());
    model.getNode("roll-deg-prop", 1).setValue(rollN.getPath());
    model.getNode("callsign-prop", 1).setValue(callsignN.getPath());
    model.getNode("description-prop", 1).setValue(descriptionN.getPath());
    model.getNode("weight-prop", 1).setValue(weightN.getPath());
    model.getNode("height-prop", 1).setValue(heightN.getPath());
    model.getNode("harness-prop", 1).setValue(harnessN.getPath());
    model.getNode("stack-prop", 1).setValue(stackN.getPath());
    model.getNode("drop-prop", 1).setValue(dropN.getPath());
    model.getNode("ai-prop", 1).setValue(aiN.getPath());
    model.getNode("load", 1).remove();

    return model;
};

################################################################################
# used for animated models
var place_animated_model = func(path, lat, lon, alt, heading, pitch, roll, name) {

	var m = props.globals.getNode("/models/JSBSimAddon/models", 1);
    for (var i = 0; 1; i += 1)
        if (m.getChild("model", i, 0) == nil)
            break;
    var model = m.getChild("model", i, 1);

    setprop("/models/JSBSimAddon/models/model["~i~"]/latitude-deg", lat);
    setprop("/models/JSBSimAddon/models/model["~i~"]/longitude-deg", lon);
    setprop("/models/JSBSimAddon/models/model["~i~"]/elevation-ft", alt * M2FT);
    setprop("/models/JSBSimAddon/models/model["~i~"]/heading-deg", heading);
    setprop("/models/JSBSimAddon/models/model["~i~"]/pitch-deg", pitch);
    setprop("/models/JSBSimAddon/models/model["~i~"]/roll-deg", roll);
    setprop("/models/JSBSimAddon/models/model["~i~"]/name", name);

	var aniModel = props.globals.getNode("/models/JSBSimAddon/models/model["~i~"]", 1);
    var latN = aniModel.getNode("latitude-deg",1);
    var lonN = aniModel.getNode("longitude-deg",1);
    var altN = aniModel.getNode("elevation-ft",1);
    var headN = aniModel.getNode("heading-deg",1);
    var pitchN = aniModel.getNode("pitch-deg",1);
    var rollN = aniModel.getNode("roll-deg",1);
    var nameN = aniModel.getNode("name",1);

    model.getNode("path", 1).setValue(path);
    model.getNode("latitude-deg-prop", 1).setValue(latN.getPath());
    model.getNode("longitude-deg-prop", 1).setValue(lonN.getPath());
    model.getNode("elevation-ft-prop", 1).setValue(altN.getPath());
    model.getNode("heading-deg-prop", 1).setValue(headN.getPath());
    model.getNode("pitch-deg-prop", 1).setValue(pitchN.getPath());
    model.getNode("roll-deg-prop", 1).setValue(rollN.getPath());
    model.getNode("name-prop", 1).setValue(nameN.getPath());
    model.getNode("load", 1).remove();

    return model;
};

