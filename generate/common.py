# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import copy
import errno
import getopt
import getpass
import imp
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import zipfile

try:
  from hashlib import sha1 as sha1
except ImportError:
  from sha import sha as sha1


def ZipWriteStr(zip, filename, data, perms=0644, compression=None):
  # use a fixed timestamp so the output is repeatable.
  zinfo = zipfile.ZipInfo(filename=filename,
                          date_time=(2009, 1, 1, 0, 0, 0))
  if compression is None:
    zinfo.compress_type = zip.compression
  else:
    zinfo.compress_type = compression
  zinfo.external_attr = perms << 16
  zip.writestr(zinfo, data)
