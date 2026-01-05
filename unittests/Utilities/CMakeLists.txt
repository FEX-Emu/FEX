add_executable(DeleteOldSHMRegions DeleteOldSHMRegions.cpp)

set_target_properties(DeleteOldSHMRegions PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TestUtilities")
add_custom_target(remove_old_shm_regions
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/TestUtilities/"
  USES_TERMINAL
  COMMAND "DeleteOldSHMRegions")

