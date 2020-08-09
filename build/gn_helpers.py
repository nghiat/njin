# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper functions useful when writing scripts that integrate with GN.

The main functions are ToGNString and FromGNString which convert between
serialized GN veriables and Python variables.

To use in a random python file in the build:

  import os
  import sys

  sys.path.append(os.path.join(os.path.dirname(__file__),
                               os.pardir, os.pardir, "build"))
  import gn_helpers

Where the sequence of parameters to join is the relative path from your source
file to the build directory."""

import sys

if sys.version_info[0] < 3:
    string_types = basestring
else:
    string_types = str

class GNException(Exception):
  pass


def ToGNString(value, allow_dicts = True):
  """Returns a stringified GN equivalent of the Python value.

  allow_dicts indicates if this function will allow converting dictionaries
  to GN scopes. This is only possible at the top level, you can't nest a
  GN scope in a list, so this should be set to False for recursive calls."""
  if isinstance(value, string_types):
    if value.find('\n') >= 0:
      raise GNException("Trying to print a string with a newline in it.")
    return '"' + \
        value.replace('\\', '\\\\').replace('"', '\\"').replace('$', '\\$') + \
        '"'

  if isinstance(value, bool):
    if value:
      return "true"
    return "false"

  if isinstance(value, list):
    return '[ %s ]' % ', '.join(ToGNString(v) for v in value)

  if isinstance(value, dict):
    if not allow_dicts:
      raise GNException("Attempting to recursively print a dictionary.")
    result = ""
    for key in sorted(value):
      if not isinstance(key, string_types):
        raise GNException("Dictionary key is not a string.")
      result += "%s = %s\n" % (key, ToGNString(value[key], False))
    return result

  if isinstance(value, int):
    return str(value)

  raise GNException("Unsupported type when printing to GN.")
