#pragma once
#define DISABLE_SPDLOG 1

#include <iostream>
#include <exception>
#ifndef DISABLE_SPDLOG
#include <spdlog/spdlog.h>
#endif
#include <string>
namespace coacd
{
    namespace logger
    {

        #ifndef DISABLE_SPDLOG
        std::shared_ptr<spdlog::logger> get();
        #else
        template <typename Arg, typename... Args>
        void doPrint(std::ostream& out, Arg&& arg, Args&&... args)
        {
            out << std::forward<Arg>(arg);
            using expander = int[];
            (void)expander{0, (void(out << ',' << std::forward<Args>(args)), 0)...};
            out << std::endl;
        }
        #endif

        template <typename... Args>
        inline void debug(std::string fmt, const Args &...args)
        {
            #ifndef DISABLE_SPDLOG
            get()->debug(fmt, args...);
            #else
            //doPrint(std::cout, fmt, args...);
            #endif
        };

        template <typename... Args>
        inline void info(std::string fmt, const Args &...args)
        {
            #ifndef DISABLE_SPDLOG
            get()->info(fmt, args...);
            #else
            //doPrint(std::cout, fmt, args...);
            #endif
        };

        template <typename... Args>
        inline void warn(std::string fmt, const Args &...args)
        {
            #ifndef DISABLE_SPDLOG
            get()->warn(fmt, args...);
            #else
            doPrint(std::cout, fmt, args...);
            #endif
        };

        template <typename... Args>
        inline void error(std::string fmt, const Args &...args)
        {
            #ifndef DISABLE_SPDLOG
            get()->error(fmt, args...);
            #else
            doPrint(std::cerr, fmt, args...);
            #endif
        };

        template <typename... Args>
        inline void critical(std::string fmt, const Args &...args)
        {
            #ifndef DISABLE_SPDLOG
            get()->critical(fmt, args...);
            #else
            doPrint(std::cerr, fmt, args...);
            #endif
        };

    } // namespace logger
} // namespace coacd
