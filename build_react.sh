#!/bin/bash
set -e  # Exit on any error

cd react-web
npx vite build
cd ..