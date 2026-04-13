# jsb_bridge.nas
#
# Bridges JSBSim internal properties into the FG property tree
# so animations can read them.

var JSBBridge = {

    new: func(jsb_root, addon_root) {

        return {
            jsb_root:   jsb_root,
            addon_root: addon_root,
            maps:       [],

            add_map: func(me, jsb_rel, addon_rel, scale) {
                if (scale == nil) scale = 1.0;

                append(me.maps, {
                    jsb:   jsb_rel,
                    addon: addon_rel,
                    scale: scale
                });
            },

            update: func(me) {
                foreach (var m; me.maps) {
                    var jsb_path   = me.jsb_root ~ "/" ~ m.jsb;
                    var addon_path = me.addon_root ~ "/" ~ m.addon;

                    var v = getprop(jsb_path);
                    if (v == nil) v = 0;

                    setprop(addon_path, v * m.scale);
                }
            },
        };
    },
};
