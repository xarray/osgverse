// osgVerse scripting
if (typeof(Module) != "undefined") {
    Module.postRun = function() {
        let verse_executer = Module.cwrap("execute", "string", ["string", "string"]);
        result = verse_executer('list', '{"library": "osg", "class": "MatrixTransform"}');
        console.log(result);
    };
}