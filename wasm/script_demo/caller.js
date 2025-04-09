// osgVerse scripting
if (typeof(Module) != "undefined") {
    Module.postRun = function() {
        let verse_executer = Module.cwrap("execute", "string", ["string", "string"], {async: true});
        async function loadOsgScene() {
            result = await verse_executer('list', '{"library": "osg", "class": "MatrixTransform"}');
            console.log(result);

            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_1/Tile_1.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_2/Tile_2.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_3/Tile_3.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_4/Tile_4.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_5/Tile_5.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_6/Tile_6.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_7/Tile_7.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_8/Tile_8.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/DATA/Tile_9/Tile_9.osgb"}');
            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');

            // TODO: simulate manipulator "HOME" to find the scene
            // TODO: other viewer / manipulator / intersection operations should also be scriptable!
        }
        loadOsgScene();
    };
}
