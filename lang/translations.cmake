macro(add_tomahawk_translations language)
    list( APPEND TOMAHAWK_LANGUAGES ${ARGV} )

    set( tomahawk_i18n_qrc_content "<!DOCTYPE RCC><RCC version=\"1.0\">\n" )

    # tomahawk and qt language files
    set( tomahawk_i18n_qrc_content "${tomahawk_i18n_qrc_content}<qresource prefix=\"/lang\">\n" )
    foreach( lang ${TOMAHAWK_LANGUAGES} )
        set( tomahawk_i18n_qrc_content "${tomahawk_i18n_qrc_content}<file>tomahawk_${lang}.qm</file>\n" )
        if( NOT lang STREQUAL "en" AND EXISTS ${QT_TRANSLATIONS_DIR}/qt_${lang}.qm )
            file( COPY ${QT_TRANSLATIONS_DIR}/qt_${lang}.qm DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
            set( tomahawk_i18n_qrc_content "${tomahawk_i18n_qrc_content}<file>qt_${lang}.qm</file>\n" )
        endif()

        # build explicitly enabled languages
        list( APPEND TS_FILES "${CMAKE_SOURCE_DIR}/lang/tomahawk_${lang}.ts" )
    endforeach()

    set( tomahawk_i18n_qrc_content "${tomahawk_i18n_qrc_content}</qresource>\n" )
    set( tomahawk_i18n_qrc_content "${tomahawk_i18n_qrc_content}</RCC>\n" )

    file( WRITE ${CMAKE_BINARY_DIR}/lang/tomahawk_i18n.qrc "${tomahawk_i18n_qrc_content}" )

    QT4_ADD_TRANSLATION( QM_FILES ${TS_FILES} )

    ## HACK HACK HACK - around rcc limitations to allow out of source-tree building
    SET( trans_file tomahawk_i18n )
    SET( trans_srcfile ${CMAKE_BINARY_DIR}/lang/${trans_file}.qrc )
    SET( trans_infile ${CMAKE_CURRENT_BINARY_DIR}/${trans_file}.qrc )
    SET( trans_outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${trans_file}.cxx )

    # Copy the QRC file to the output directory
    ADD_CUSTOM_COMMAND(
        OUTPUT ${trans_infile}
        COMMAND ${CMAKE_COMMAND} -E copy ${trans_srcfile} ${trans_infile}
        MAIN_DEPENDENCY ${trans_srcfile}
    )

    # Run the resource compiler (rcc_options should already be set)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${trans_outfile}
        COMMAND ${QT_RCC_EXECUTABLE}
        ARGS ${rcc_options} -name ${trans_file} -o ${trans_outfile} ${trans_infile}
        MAIN_DEPENDENCY ${trans_infile}
        DEPENDS ${QM_FILES}
    )
endmacro()

