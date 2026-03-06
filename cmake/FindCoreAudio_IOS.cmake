# Copyright (c) 2015 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# COREAUDIO_FOUND
# COREAUDIO_INCLUDE_DIR
# COREAUDIO_LIBRARY

find_path(COREAUDIO_IOS_INCLUDE_DIR NAMES CoreAudio/CoreAudioTypes.h)

find_library(COREAUDIO_IOS_LIBRARY NAMES CoreAudio)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(COREAUDIO_IOS DEFAULT_MSG COREAUDIO_IOS_LIBRARY COREAUDIO_IOS_INCLUDE_DIR)

mark_as_advanced(COREAUDIO_IOS_INCLUDE_DIR COREAUDIO_IOS_LIBRARY)
