#!/bin/bash
# Script Name: magick-ico
# Description: This script converts icon.png into multiple sized icons compressed into a single file.
# Source: https://stackoverflow.com/questions/3236115/which-icon-sizes-should-my-windows-applications-icon-include
set -uo pipefail

# Convert PNG to a multi icon file
magick icon.png -background transparent -define icon:auto-resize="16,24,32,48,64,96,128,256" lmuffb.ico
