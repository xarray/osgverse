// osgVerse scripting
if (typeof(Module) != "undefined") {
    Module.postRun = function() {
        let verse_executer = Module.cwrap("execute", "string", ["string", "string"], {async: true});
        async function loadOsgScene() {
            result = await verse_executer('list', '{"library": "osg", "class": "MatrixTransform"}');
            console.log(result);

            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+958_+8053/Tile_+958_+8053.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+958_+8054/Tile_+958_+8054.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+958_+8055/Tile_+958_+8055.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+959_+8053/Tile_+959_+8053.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+959_+8054/Tile_+959_+8054.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+959_+8055/Tile_+959_+8055.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+960_+8053/Tile_+960_+8053.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+960_+8054/Tile_+960_+8054.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+960_+8055/Tile_+960_+8055.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');

            // TODO: simulate manipulator "HOME" to find the scene
            // TODO: other viewer / manipulator / intersection operations should also be scriptable!
        }
        loadOsgScene();
    };
}
