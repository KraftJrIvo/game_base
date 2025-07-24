function(embed_resources RESOURCE_DIR OUT_VAR)
    cmake_parse_arguments(ARG "" "" "EXCLUDE_EXTENSIONS" ${ARGN})

    file(GLOB_RECURSE RESOURCE_FILES RELATIVE "${RESOURCE_DIR}" "${RESOURCE_DIR}/*")

    set(GENERATED_C_FILES)
    set(RESOURCE_DECLS)

    foreach(RESOURCE_FILE ${RESOURCE_FILES})
        get_filename_component(FILE_EXT "${RESOURCE_FILE}" EXT)

        # Exclude extensions
        set(SKIP_FILE FALSE)
        foreach(EXCLUDED_EXT ${ARG_EXCLUDE_EXTENSIONS})
            if(FILE_EXT STREQUAL "${EXCLUDED_EXT}")
                set(SKIP_FILE TRUE)
                break()
            endif()
        endforeach()
        if(SKIP_FILE)
            message(STATUS "Skipping resource: ${RESOURCE_FILE} (excluded extension)")
            continue()
        endif()

        # Create symbol name: res_ui_icons_play_png
        string(REPLACE "/" "_" SYMBOL_PATH "${RESOURCE_FILE}")
        string(REPLACE "." "_" SYMBOL_NAME "${SYMBOL_PATH}")
        set(SYMBOL_NAME "res_${SYMBOL_NAME}")

        # Output .c file
        set(OUTPUT_C "${CMAKE_CURRENT_BINARY_DIR}/${SYMBOL_NAME}.c")

        add_custom_command(
            OUTPUT "${OUTPUT_C}"
            COMMAND embedfile "${SYMBOL_NAME}" "${RESOURCE_DIR}/${RESOURCE_FILE}"
            DEPENDS "${RESOURCE_DIR}/${RESOURCE_FILE}"
            COMMENT "Embedding ${RESOURCE_FILE} as ${SYMBOL_NAME}"
            VERBATIM
        )

        # Append to generated files list
        list(APPEND GENERATED_C_FILES "${OUTPUT_C}")

        # Add to extern declarations
        string(APPEND RESOURCE_DECLS "extern const unsigned char ${SYMBOL_NAME}[];\n")
        string(APPEND RESOURCE_DECLS "extern const size_t ${SYMBOL_NAME}_len;\n\n")
    endforeach()

    # Write the resources.c file containing extern declarations
    set(RESOURCES_C "${CMAKE_CURRENT_BINARY_DIR}/resources.c")
    file(WRITE "${RESOURCES_C}" "// Auto-generated resource declarations\n\n${RESOURCE_DECLS}")
    list(APPEND GENERATED_C_FILES "${RESOURCES_C}")

    # Return full list of .c files
    set(${OUT_VAR} "${GENERATED_C_FILES}" PARENT_SCOPE)

    # At the end of embed_resources function, after building RESOURCE_DECLS
    set(RESOURCES_C "${CMAKE_CURRENT_BINARY_DIR}/resources.c")
    set(RESOURCES_H "${CMAKE_CURRENT_BINARY_DIR}/resources.h")

    file(WRITE "${RESOURCES_H}" "// Auto-generated resource header\n\n")
    file(APPEND "${RESOURCES_H}" "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
    file(APPEND "${RESOURCES_H}" "${RESOURCE_DECLS}")
    file(APPEND "${RESOURCES_H}" "#ifdef __cplusplus\n}\n#endif\n")

    list(APPEND GENERATED_C_FILES "${RESOURCES_C}")
    # Also expose RESOURCES_H so it can be included in target's include path
    set(${OUT_VAR} "${GENERATED_C_FILES}" PARENT_SCOPE)

    # Optional: export header path so user can include it
    set(RESOURCES_HEADER "${RESOURCES_H}" PARENT_SCOPE)

    set_source_files_properties(${GENERATED_C_FILES} PROPERTIES GENERATED TRUE)
    
endfunction()
