Project(server_service)

add_library(server_service
    $<TARGET_OBJECTS:manager_service_obj>
    $<TARGET_OBJECTS:login_service_obj>
    $<TARGET_OBJECTS:game_service_obj>
)
