#!/bin/bash
set -e  # Exit on any error

cd web
npm run build
cd ..