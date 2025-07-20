# Documentation

if(Doxygen_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    # Install documentation
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
        OPTIONAL
    )
else()
    message(WARNING "Doxygen not found, documentation will not be built")
endif()
