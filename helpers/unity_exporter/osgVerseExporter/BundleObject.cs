using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleObject
    {
        public object GetObjectData()
        {
            throw new NotImplementedException("Attempting to call GetObjectData");
        }

        public string name;
    }

}
#endif