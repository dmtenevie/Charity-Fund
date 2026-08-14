cmake_minimum_required(VERSION 3.16)
project(CharityFund VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql Charts PrintSupport)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    src/main.cpp
    src/Theme.cpp
    src/Lang.cpp
    src/gui/MainWindow.cpp
    src/gui/AppIcon.cpp
    src/gui/DashboardWidget.cpp
    src/gui/DonorsWidget.cpp
    src/gui/DonationsWidget.cpp
    src/gui/ProjectsWidget.cpp
    src/gui/BeneficiariesWidget.cpp
    src/gui/ReportsWidget.cpp
    src/models/Donor.cpp
    src/models/Donation.cpp
    src/models/Project.cpp
    src/models/Beneficiary.cpp
    src/database/Database.cpp
    src/database/Repository.cpp
    resources/resources.qrc
)

set(HEADERS
    src/Theme.h
    src/Lang.h
    src/gui/MainWindow.h
    src/gui/AppIcon.h
    src/gui/DashboardWidget.h
    src/gui/DonorsWidget.h
    src/gui/DonationsWidget.h
    src/gui/ProjectsWidget.h
    src/gui/BeneficiariesWidget.h
    src/gui/ReportsWidget.h
    src/models/Donor.h
    src/models/Donation.h
    src/models/Project.h
    src/models/Beneficiary.h
    src/database/Database.h
    src/database/Repository.h
)

add_executable(charity_fund WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(charity_fund
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::PrintSupport
)

if(WIN32)
    target_link_libraries(charity_fund dwmapi)
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Build Configuration")
message(STATUS "========================================")
message(STATUS "Qt6:     ${Qt6_VERSION}")
message(STATUS "Driver:  Qt QPSQL (PostgreSQL)")
message(STATUS "========================================")
message(STATUS "")

# Installation
install(TARGETS charity_fund
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
