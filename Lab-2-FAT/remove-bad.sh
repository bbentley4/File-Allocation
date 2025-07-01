#!/bin/bash

# Find and remove all files starting with ._
find . -type f -name '._*' -exec rm -f {} +

echo "All files starting with ._ have been removed."
