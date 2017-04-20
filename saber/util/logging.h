// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_LOGGING_H_
#define SABER_UTIL_LOGGING_H_

#include <inttypes.h>
#include <stdarg.h>

namespace saber {

enum LogLevel {
  LOGLEVEL_DEBUG,
  LOGLEVEL_INFO,
  LOGLEVEL_WARN,
  LOGLEVEL_ERROR,
  LOGLEVEL_FATAL
};

extern void Log(LogLevel level, const char* filename, int line,
                const char* format, ...)
#    if defined(__GNUC__) || defined(__clang__)
     __attribute__((__format__ (__printf__, 4, 5)))
#    endif
     ;

#define LOG_DEBUG(format, ...)  \
  ::saber::Log(                 \
    ::saber::LOGLEVEL_DEBUG, __FILE__, __LINE__, format, ## __VA_ARGS__)

#define LOG_INFO(format, ...)  \
  ::saber::Log(                 \
    ::saber::LOGLEVEL_INFO, __FILE__, __LINE__, format, ## __VA_ARGS__)

#define LOG_WARN(format, ...)  \
  ::saber::Log(                 \
    ::saber::LOGLEVEL_WARN, __FILE__, __LINE__, format, ## __VA_ARGS__)

#define LOG_ERROR(format, ...)  \
  ::saber::Log(                 \
    ::saber::LOGLEVEL_ERROR, __FILE__, __LINE__, format, ## __VA_ARGS__)

#define LOG_FATAL(format, ...)  \
  ::saber::Log(                 \
    ::saber::LOGLEVEL_FATAL, __FILE__, __LINE__, format, ## __VA_ARGS__)

extern void DefaultLogHandler(LogLevel level, const char* filename, int line,
                              const char* format, va_list ap);

typedef void LogHandler(LogLevel level,
                        const char* filename, int line,
                        const char* format, va_list ap);

extern LogHandler* SetLogHandler(LogHandler* new_handler);

extern LogLevel SetLogLevel(LogLevel new_level);

}  // namespace saber

#endif  // SABER_UTIL_LOGGING_H_
