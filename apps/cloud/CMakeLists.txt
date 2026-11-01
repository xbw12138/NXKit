# cloud: cloud drive client built on top of nxkit_core.
nxkit_add_app(cloud
        TITLEID "0x010FF000FFFF0102"
        AUTHOR "nxkit"
        SOURCES
        source/main.cpp
        source/main_activity.cpp
        source/drive/drive_manager.cpp
        source/drive/drive_provider.cpp
        source/drive/quark_drive.cpp
        source/pages/login_page.cpp
        source/pages/file_list_page.cpp
        source/pages/profile_page.cpp
        source/pages/player_page.cpp
        source/pages/image_viewer.cpp
        )
