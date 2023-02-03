
#project(PSI)


#############################################
#                 Link cryptoTools          #
#############################################

find_library(
    cryptoTools_LIB
    NAMES cryptoTools
    HINTS "${libOTe_Dirs}/out/build/linux/cryptoTools/cryptoTools/")
    
# if we cant fint it, throw an error
if(NOT cryptoTools_LIB)
  Message(${cryptoTools_LIB})
  message(FATAL_ERROR "Failed to find cryptoTools at ")
else()
  message("cryptoTools at  ${MIRACL_LIB}")
endif()

#############################################
#                 Link libOTe               #
#############################################

find_library(
    libOTe_LIB
    NAMES libOTe
    HINTS "${libOTe_Dirs}/out/build/linux/libOTe/")
    
# if we cant fint it, throw an error
if(NOT libOTe_LIB)
  Message(${libOTe_LIB})
  message(FATAL_ERROR "Failed to find libOTe at ")
else()
  message("libOTe at  ")
endif()

#############################################
#              Build PSI                    #
#############################################

file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
 

add_library(PSI STATIC ${SRCS})
target_include_directories(PSI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include  ${libOTe_Dirs}/cryptoTools ${libOTe_Dirs} ${libOTe_Dirs}/libOTe)

target_link_libraries(PSI ${libOTe_LIB} ${cryptoTools_LIB})

