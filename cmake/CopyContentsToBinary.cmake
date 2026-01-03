function(copy_contents_to_binary TARGET SOURCE_DIR)
    if (EXISTS "${SOURCE_DIR}")
        add_custom_command(
            TARGET ${TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${SOURCE_DIR}/"
                    "$<TARGET_FILE_DIR:${TARGET}>"
            COMMENT "Copying contents of ${SOURCE_DIR} to target output directory"
        )
    else()
        message(STATUS "Folder does not exist, skipping copy: ${SOURCE_DIR}")
    endif()
endfunction()
