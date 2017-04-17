list(APPEND WTF_SOURCES
    PlatformUserPreferredLanguagesUnix.cpp

    generic/WorkQueueGeneric.cpp
)

if (WIN32)
    list(APPEND WTF_SOURCES
        text/win/TextBreakIteratorInternalICUWin.cpp
    )
else ()
    list(APPEND WTF_SOURCES
        text/unix/TextBreakIteratorInternalICUUnix.cpp
    )
endif ()

if (LOWERCASE_EVENT_LOOP_TYPE STREQUAL "glib")
    list(APPEND WTF_SOURCES
        glib/GRefPtr.cpp
        glib/MainThreadGLib.cpp
        glib/RunLoopGLib.cpp
    )
    list(APPEND WTF_SYSTEM_INCLUDE_DIRECTORIES
        ${GLIB_INCLUDE_DIRS}
    )
    list(APPEND WTF_LIBRARIES
        ${GLIB_GIO_LIBRARIES}
        ${GLIB_GOBJECT_LIBRARIES}
        ${GLIB_LIBRARIES}
    )
else ()
    list(APPEND WTF_SOURCES
        generic/MainThreadGeneric.cpp
        generic/RunLoopGeneric.cpp
    )
endif ()

list(APPEND WTF_LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
)

if (APPLE)
    list(APPEND WTF_INCLUDE_DIRECTORIES
        "${WTF_DIR}/icu"
    )
endif ()
