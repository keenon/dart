# Search all header and source files
file(GLOB hdrs "*.hpp")
file(GLOB srcs "*.cpp")
file(GLOB detail_hdrs "detail/*.hpp")
file(GLOB detail_srcs "detail/*.cpp")
dart_add_core_headers(${hdrs} ${detail_hdrs})
dart_add_core_sources(${srcs} ${detail_srcs})

# Generate header for this namespace
dart_get_filename_components(header_names "dynamics headers" ${hdrs})
# TODO: remove below line once the files are completely removed.
list(REMOVE_ITEM header_names "MultiSphereShape.hpp")
dart_generate_include_header_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dynamics.hpp"
  "dart/dynamics/"
  ${header_names}
)

# Install
install(
  FILES ${hdrs} ${CMAKE_CURRENT_BINARY_DIR}/dynamics.hpp
  DESTINATION include/dart/dynamics
  COMPONENT headers
)
install(
  FILES ${detail_hdrs}
  DESTINATION include/dart/dynamics/detail
  COMPONENT headers
)

dart_format_add(
  HeightmapShape.hpp
  PointCloudShape.hpp
  PointCloudShape.cpp
  SmartPointer.hpp
  TranslationalJoint2D.hpp
  TranslationalJoint2D.cpp
  detail/HeightmapShape-impl.hpp
  detail/TranslationalJoint2DAspect.hpp
  detail/TranslationalJoint2DAspect.cpp
)
