set(FM_CORE_SOURCES
    core/component/IComponent.h
    core/component/MissionComponent.h
    core/component/MovementComponent.h
    core/component/SensorComponent.h
    core/entity/Entity.h
    core/entity/Entity.cpp
    core/math/Vector2.h
    core/simulation/SimulationManager.h
    core/simulation/SimulationManager.cpp
    core/simulation/SimulationUpdateContext.h
)

set(FM_APP_SOURCES
    app/scenario/ScenarioDefinition.h
    app/scenario/ScenarioLoader.h
    app/scenario/ScenarioLoader.cpp
    app/services/SimulationSession.h
    app/services/SimulationSession.cpp
)

set(FM_UI_SOURCES
    ui/widgets/MainWindow.h
    ui/widgets/MainWindow.cpp
    main/main.cpp
)

add_library(fm_core ${FM_CORE_SOURCES})
target_include_directories(fm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(fm_app ${FM_APP_SOURCES})
target_include_directories(fm_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fm_app PUBLIC fm_core Qt5::Core)

add_executable(FunctionalModelTest ${FM_UI_SOURCES})
target_include_directories(FunctionalModelTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(FunctionalModelTest PRIVATE fm_app Qt5::Core Qt5::Gui Qt5::Widgets)

add_custom_command(
    TARGET FunctionalModelTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:FunctionalModelTest>/assets
)