using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    static class ExportError
    {
        public static void FatalError(string message)
        {
            EditorUtility.ClearProgressBar();
            EditorUtility.DisplayDialog("Error", message, "Ok");
            throw new Exception(message);
        }
    }

}
#endif