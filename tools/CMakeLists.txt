add_subdirectory(mxl-info)
add_subdirectory(mxl-gst)

if (MXL_FABRICS_OFI)
    add_subdirectory(mxl-fabrics-demo)
endif()
