build_example(
  NAME scenario1
  SOURCE_FILES scenario1.cc
  LIBRARIES_TO_LINK "${ns3-libs}" "${ns3-contrib-libs}"
)