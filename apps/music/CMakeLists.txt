# music: the sample app built on top of nxkit_core.
nxkit_add_app(music
        TITLEID "0x010FF000FFFF0101"
        AUTHOR "nxkit"
        SOURCES
        source/main.cpp
        source/main_activity.cpp
        source/netease_client.cpp
        source/pages/browse_page.cpp
        source/pages/search_page.cpp
        source/pages/profile_page.cpp
        source/pages/login_page.cpp
        )
