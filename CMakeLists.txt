cmake_minimum_required(VERSION 3.16)
project(FourColor_E_ink_Driver LANGUAGES C)

# Generic C library; ESP-IDF projects use the component directory instead.
add_library(mspq1tmn1001qnx STATIC
    components/mspq1tmn1001qnx_epd/mspq1tmn1001qnx.c)
target_include_directories(mspq1tmn1001qnx PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/components/mspq1tmn1001qnx_epd/include")
target_compile_features(mspq1tmn1001qnx PUBLIC c_std_99)
add_library(FourColorEink::Driver ALIAS mspq1tmn1001qnx)
