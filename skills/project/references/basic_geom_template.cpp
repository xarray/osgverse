// ============================================
// Geometry/网格体对象创建标准模板
// @skill-status: partial
// @skill-category: geometry
// @dependencies: osgVerseModeling
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/modeling/Math.h>
#include <osgVerse/modeling/Utilities.h>

// 创建任意复杂度的几何体对象
{
    // 使用智能指针ref_ptr管理新建的顶点属性对象
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;  // 顶点位置
    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;  // 顶点UV坐标
    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;  // 顶点法线（可选）

    // 根据需要添加足够多的顶点位置信息
    va->push_back(osg::Vec3(-1.0f, -1.0f, -1.0f));
    va->push_back(osg::Vec3(1.0f, -1.0f, -1.0f));
    va->push_back(osg::Vec3(1.0f, 1.0f, -1.0f));
    va->push_back(osg::Vec3(-1.0f, 1.0f, -1.0f));
    // TODO

    // 添加顶点UV坐标，注意其数量一定与顶点位置va的数量一致
    ta->push_back(osg::Vec2(0.0f, 0.0f));
    ta->push_back(osg::Vec2(1.0f, 0.0f));
    ta->push_back(osg::Vec2(1.0f, 1.0f));
    ta->push_back(osg::Vec2(0.0f, 1.0f));
    // TODO

    // 如有数据，则继续添加顶点法线，注意其数量一定与顶点位置va的数量一致
    na->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    na->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    na->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    na->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    // TODO

    // 为了确保图形接口的兼容性，几何体的图元类型只能是：GL_POINTS（点集），GL_LINES/GL_LINE_STRIP（线段），GL_TRIANGLES/GL_TRIANGLE_STRIP（三角形）
    // 如果不需要复用顶点数据来建立图元索引集，则直接使用osg::DrawArrays来构建图元信息
    if (noIndices)
    {
        osg::ref_ptr<osg::PrimitiveSet> ps = new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, va->size());

        // 如果没有输入法线数组，需要自动计算法线，则设置末尾参数为true；否则传入法线数组并设置末尾参数为false
        osg::ref_ptr<osg::Geometry> geom = osgVerse::createGeometry(va.get(), na.get(), ta.get(), ps.get(), false);

        // TODO: 执行几何体创建后的操作
    }

    // 根据图元索引元素的总数，使用osg::DrawElementsUShort或者osg::DrawElementsUInt来构建图元索引信息
    else
    {
        // 根据需要添加足够多的索引参数，注意索引值一定要大于0，小于顶点位置va的总数量
        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
        de->push_back(0); de->push_back(1); de->push_back(2);  // 三角形1
        de->push_back(0); de->push_back(2); de->push_back(3);  // 三角形2
        // TODO

        // 如果没有输入法线数组，需要自动计算法线，则设置末尾参数为true；否则传入法线数组并设置末尾参数为false
        osg::ref_ptr<osg::Geometry> geom = osgVerse::createGeometry(va.get(), na.get(), ta.get(), de.get(), false);

        // TODO: 执行几何体创建后的操作
    }

    // TODO：更多的执行代码...
}
