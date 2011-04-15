FILE (GLOB TS_FILES ${CMAKE_SOURCE_DIR}/lang/*.ts)
QT4_ADD_TRANSLATION(QM_FILES ${TS_FILES})

## HACK HACK HACK - around rcc limitations to allow out of source-tree building
SET( trans_file tomahawk_i18n )
SET( trans_srcfile ${CMAKE_SOURCE_DIR}/lang/${trans_file}.qrc)
SET( trans_infile ${CMAKE_CURRENT_BINARY_DIR}/${trans_file}.qrc)
SET( trans_outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${trans_file}.cxx)

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

