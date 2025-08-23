// osgVerse scripting
if (typeof(Module) != "undefined") {
    Module.postRun = function() {
        let verse_executer = Module.cwrap("execute", "string", ["string", "string"], {async: true});
        async function loadOsgScene() {
            result = await verse_executer('list', '{"library": "osg", "class": "MatrixTransform"}');
            console.log(result);

            // TODO: simulate manipulator "HOME" to find the scene
            // TODO: other viewer / manipulator / intersection operations should also be scriptable!
        }
        loadOsgScene();
    };
}
