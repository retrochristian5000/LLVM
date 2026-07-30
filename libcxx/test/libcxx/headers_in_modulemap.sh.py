# RUN: %{python} %s %{libcxx-dir}/utils %{include-dir}

import pathlib
import sys
sys.path.append(sys.argv[1])
from libcxx.header_information import all_headers

include = pathlib.Path(sys.argv[2])
with open(include / "module.modulemap") as f:
    modulemap = f.read()

isHeaderMissing = False
for header in all_headers:
    if not header.is_in_modulemap():
        continue

    if not str(header) in modulemap:
        print(f"Header {header} seems to be missing from the modulemap!")
        isHeaderMissing = True

if isHeaderMissing:
    exit(1)
