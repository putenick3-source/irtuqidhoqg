@echo off
cd /d "%~dp0"
title ROB RUN loader
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0loader.ps1"
