// osgVerse scripting
if (typeof(Module) != "undefined") {
    Module.postRun = function() {
        let verse_executer = Module.cwrap("execute", "string", ["string", "string"], {async: true});
        async function loadOsgScene() {
            result = await verse_executer('list', '{"library": "osg", "class": "MatrixTransform"}');
            console.log(result);

            // NOT WORKING: https://github.com/emscripten-core/emscripten/issues/12239  // TODO: emsdk should be updated first!!
            //result = await verse_executer('create', '{"type": "node", "uri": "http://127.0.0.1:8000/assets/Data/Tile_+958_+8053/Tile_+958_+8053.osgb"}');
            result = await verse_executer('create', '{"class": "osg::MatrixTransform"}');
            console.log(result);

            result = await verse_executer('set', '{"object": "root", "method": "addChild", "properties": ["' + JSON.parse(result).value + '"]}');
            console.log(result);
        }
        loadOsgScene();
    };
}
