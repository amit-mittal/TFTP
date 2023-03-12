# CMakeList.txt: CMake-Projektdatei der obersten Ebene. Führen Sie hier die globale Konfiguration aus,
# und schließen Sie Unterprojekte ein.
#
cmake_minimum_required (VERSION 3.8)

project ("TFTP_Master")

add_executable (TFTP_Master "tftp_s.c" "utility.h" "win_unistd.h")

if(WIN32)
  target_link_libraries(TFTP_Master wsock32 ws2_32)
endif()

add_executable (TFTP_Client "tftp_c.c" "utility.h" "win_unistd.h")

if(WIN32)
  target_link_libraries(TFTP_Client wsock32 ws2_32)
endif()

# Schließen Sie Unterprojekte ein.
#add_subdirectory ("CMakeProject1")
