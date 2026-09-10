#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#ifdef _WIN32
#   include <windows.h>
#   include <io.h>
#   define popen  _popen
#   define pclose _pclose
#   define isatty _isatty
#   define fileno _fileno
#else
#   include <unistd.h>
#endif

namespace fs = std::filesystem;
static bool g_colorEnabled = false;
#define COLOR_RED_BEGIN  if (g_colorEnabled) std::cout << "\033[31;1m"
#define COLOR_RED_END    if (g_colorEnabled) std::cout << "\033[0m"

void setupColor()
{
    if (!isatty(fileno(stdout))) return;
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        g_colorEnabled = true;
    }
#else
    g_colorEnabled = true;
#endif
}

std::string runCommand(const std::string& cmd)
{
    std::string result; char buffer[4096];
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
    pclose(pipe); return result;
}

void splitString(const std::string& s, char delim, std::vector<std::string>& out)
{
    std::stringstream ss(s); std::string item;
    while (std::getline(ss, item, delim))
        if (!item.empty()) out.push_back(item);
}

#ifdef _WIN32   // Windows: 'dumpbin /dependents'
std::vector<std::string> getDirectDependencies(const std::string& libPath)
{
    std::vector<std::string> deps;
    std::string output = runCommand("dumpbin /dependents \"" + libPath + "\" 2>NUL");

    std::istringstream iss(output); std::string line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() > 4 && line.compare(0, 4, "    ") == 0 &&
            line.find(".dll") != std::string::npos)
        {
            std::string name = line.substr(4);
            size_t end = name.find_last_not_of(" \t\r\n");
            if (end != std::string::npos) name.erase(end + 1);
            if (!name.empty()) deps.push_back(name);
        }
    }
    return deps;
}
#else  // Linux: 'readelf -d' or ldd
std::vector<std::string> getDirectDependencies(const std::string& libPath)
{
    std::vector<std::string> deps;
    std::string output = runCommand("readelf -d \"" + libPath + "\" 2>/dev/null");

    std::istringstream iss(output); std::string line;
    while (std::getline(iss, line))
    {
        size_t pos = line.find("NEEDED");
        if (pos != std::string::npos)
        {
            size_t start = line.find('[', pos), end = line.find(']', pos);
            if (start != std::string::npos && end != std::string::npos)
                deps.push_back(line.substr(start + 1, end - start - 1));
        }
    }
    
    if (deps.empty())
    {
        output = runCommand("ldd \"" + libPath + "\" 2>/dev/null");
        std::istringstream iss2(output);
        while (std::getline(iss2, line))
        {
            size_t pos = line.find("=>");
            if (pos == std::string::npos) continue;
            
            std::string name = line.substr(0, pos);
            size_t end = name.find_last_not_of(" \t");
            if (end != std::string::npos) name.erase(end + 1);
            if (!name.empty()) deps.push_back(name);
        }
    }
    return deps;
}
#endif

static std::vector<fs::path> g_searchPaths;
static std::unordered_map<std::string, fs::path> g_foundCache;   // name -> path
static std::unordered_set<std::string> g_missingCache;
static bool g_checkArchTriplet = true;
#if defined(__x86_64__)
    #define DEP_ARCH_TRIPLET "x86_64-linux-gnu"
#elif defined(__aarch64__)
    #define DEP_ARCH_TRIPLET "aarch64-linux-gnu"
#elif defined(__arm__)
    #define DEP_ARCH_TRIPLET "arm-linux-gnueabihf"
#elif defined(__i386__)
    #define DEP_ARCH_TRIPLET "i386-linux-gnu"
#else
    #define DEP_ARCH_TRIPLET ""
    g_checkArchTriplet = false;
#endif

void initSearchPaths(const std::string& startDir)
{
    if (!startDir.empty()) g_searchPaths.push_back(startDir);
#ifdef _WIN32
    // Windows DLL searching paths: Current -> System32 -> Windows -> PATH
    char buf[32768];
    if (GetSystemDirectoryA(buf, sizeof(buf))) g_searchPaths.push_back(fs::path(buf));
    if (GetWindowsDirectoryA(buf, sizeof(buf))) g_searchPaths.push_back(fs::path(buf));
    if (GetEnvironmentVariableA("PATH", buf, sizeof(buf)))
    {
        std::vector<std::string> parts; splitString(buf, ';', parts);
        for (size_t i = 0; i < parts.size(); ++i)
            g_searchPaths.push_back(fs::path(parts[i]));
    }
#else
    // Linux searching paths: LD_LIBRARY_PATH
    const char* ld = getenv("LD_LIBRARY_PATH");
    if (ld)
    {
        std::vector<std::string> parts; splitString(ld, ':', parts);
        for (size_t i = 0; i < parts.size(); ++i)
            g_searchPaths.push_back(fs::path(parts[i]));
    }
    
    std::string ldout = runCommand("ldconfig -p 2>/dev/null");
    std::istringstream iss(ldout); std::string line;
    while (std::getline(iss, line))
    {
        size_t arrow = line.find("=> ");
        if (arrow != std::string::npos)
        {
            size_t space = line.find(' ');
            if (space != std::string::npos)
                g_foundCache[line.substr(0, space)] = fs::path(line.substr(arrow + 3));
        }
    }
    g_searchPaths.push_back(fs::path("/lib64"));
    g_searchPaths.push_back(fs::path("/usr/lib64"));
    g_searchPaths.push_back(fs::path("/lib"));
    g_searchPaths.push_back(fs::path("/usr/lib"));
    g_searchPaths.push_back(fs::path("/usr/local/lib64"));
    g_searchPaths.push_back(fs::path("/usr/local/lib"));
    if (g_checkArchTriplet)
    {
        g_searchPaths.push_back(fs::path("/lib/" DEP_ARCH_TRIPLET));
        g_searchPaths.push_back(fs::path("/usr/lib/" DEP_ARCH_TRIPLET));
        g_searchPaths.push_back(fs::path("/usr/local/lib/" DEP_ARCH_TRIPLET));
    }
#endif
}

bool resolveLibrary(const std::string& name, fs::path& outPath)
{
    std::unordered_map<std::string, fs::path>::iterator it = g_foundCache.find(name);
    if (it != g_foundCache.end()) { outPath = it->second; return true; }
    if (g_missingCache.count(name)) return false;

    for (size_t i = 0; i < g_searchPaths.size(); ++i)
    {
        fs::path candidate = g_searchPaths[i] / name; std::error_code ec;
        if (fs::exists(candidate, ec))
            { outPath = candidate; g_foundCache[name] = candidate; return true; }
    }
    g_missingCache.insert(name);
    return false;
}

#ifndef _WIN32
void getElfRunPath(const std::string& libPath, bool rpathOnly,
                   std::vector<std::string>& dirs)
{
    std::string output = runCommand("readelf -d \"" + libPath + "\" 2>/dev/null");
    std::istringstream iss(output); std::string line;
    
    bool hasRunPath = false;
    while (std::getline(iss, line))
    { if (line.find("RUNPATH") != std::string::npos) {hasRunPath = true; break;} }

    iss.clear(); iss.seekg(0);
    while (std::getline(iss, line))
    {
        // 0x000000000000001d (RPATH)   Library rpath: [/opt/foo/lib:/lib64]
        bool isRPath   = line.find("(RPATH)")   != std::string::npos;
        bool isRunPath = line.find("(RUNPATH)") != std::string::npos;
        if (rpathOnly ? !isRPath : !isRunPath) continue;
        if (isRPath && hasRunPath) continue;

        size_t start = line.find('[', line.find("Library"));
        size_t end   = line.find(']', start);
        if (start == std::string::npos || end == std::string::npos) continue;

        std::vector<std::string> raw;
        splitString(line.substr(start + 1, end - start - 1), ':', raw);
        fs::path libDir = fs::path(libPath).parent_path();
        for (size_t i = 0; i < raw.size(); ++i)
        {
            std::string d = raw[i];  // find ${ORIGIN} or $ORIGIN
            for (;;)
            {
                size_t p = d.find("$ORIGIN");
                if (p == std::string::npos) p = d.find("${ORIGIN}");
                if (p == std::string::npos) break;
                size_t len = (d[p+1] == '{') ? 9 : 7;
                d.replace(p, len, libDir.string());
            }
            dirs.push_back(d);
        }
    }
}
#endif

void printDeps(const std::string& name, const fs::path& path,
               int depth, std::unordered_set<std::string>& visited)
{
    std::string indent(static_cast<size_t>(depth) * 4, ' ');
    if (depth > 0)
    {
        std::string display = path.empty() ? "???" : path.parent_path().string();
        std::cout << indent << "- " << name << ": " << display;
        if (path.empty())
        {
            std::cout << " "; COLOR_RED_BEGIN;
            std::cout << "(missing)"; COLOR_RED_END;
        }
        std::cout << "\n";
    } else
        std::cout << name << ": " << path.parent_path().string() << "\n";
    if (!visited.insert(name).second) return;

    size_t oldSearchCount = g_searchPaths.size();
#ifndef _WIN32
    if (!path.empty())
    {
        std::vector<std::string> rdirs, rpdirs;
        getElfRunPath(path.string(), false, rdirs);   // RUNPATH
        if (rdirs.empty())
            getElfRunPath(path.string(), true, rpdirs); // No RUNPATH, try finding RPATH

        std::vector<std::string>& dirs = rdirs.empty() ? rpdirs : rdirs;
        for (std::vector<std::string>::reverse_iterator it = dirs.rbegin();
             it != dirs.rend(); ++it)
        g_searchPaths.insert(g_searchPaths.begin(), fs::path(*it));
    }
#endif

    std::vector<std::string> deps = getDirectDependencies(path.string());
    for (size_t i = 0; i < deps.size(); ++i)
    {
        fs::path depPath; resolveLibrary(deps[i], depPath);
        printDeps(deps[i], depPath, depth + 1, visited);
    }
    g_searchPaths.resize(oldSearchCount);
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    std::string pluginExt, libName;
    if (arguments.read("--ext", pluginExt))
    {
        osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(pluginExt);
        if (rw != NULL) { std::cout << rw->className() << " loaded.\n"; return 0; }
        libName = osgDB::Registry::instance()->createLibraryNameForExtension(pluginExt);
    }
    else if (argc > 1) libName = argv[1];
    else { std::cout << "Usage: osgVerse_Test_Plugins <extension_name or lib_name>\n"; return 1; }

    osgDB::Registry::LoadStatus status = osgDB::Registry::instance()->loadLibrary(libName);
    if (status != osgDB::Registry::NOT_LOADED) std::cout << libName << " loaded.\n";
    else std::cout << "!!! Failed to load " << libName << ".\n";
    
    setupColor();
    fs::path libPath; std::error_code ec;
    if (fs::exists(libName, ec))
        libPath = fs::absolute(libName, ec);
    else
    {
        initSearchPaths("");
        if (!resolveLibrary(libName, libPath))
            { std::cerr << "Cannot find library: " << libName << "\n"; return 1; }
    }

    std::unordered_set<std::string> visited;
    initSearchPaths(libPath.parent_path().string());
    for (int i = 2; i < argc; ++i)
        g_searchPaths.push_back(fs::path(argv[i]));
    printDeps(libName, libPath, 0, visited);
    return 0;
}
