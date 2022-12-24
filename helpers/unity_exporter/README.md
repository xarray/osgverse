# osgVerse Unity exporter

#### Introduction
This is a Unity exporter to convert Unity built-in (at present) scene into .osg format for loading and rendering in osgVerse.
All PBR textures will be converted and copied, as well as geometries and other scene components

#### Installation
1. Put the 'osgVerseExporter' folder into your Unity project's 'Assets' folder.
2. Wait for Unity Editor to compile automatically.
3. A new menu item 'osgVerse' will appear at the menu-bar.
4. Select 'Export All' or 'Export selected' to export Unity scene in .osg format

#### TODO
1. Support geometry data and transformation (done)
2. Support lightmaps and lightprobes (partly)
3. Support PBR textures, especially Occlusion/Roughness/Metallic (done)
4. Support light and camera data outputs
5. Support game object and camera animations
6. Support character data and animation outputs
7. Support mesh collider data outputs
8. Support particle animations
9. Support terrains (partly)
