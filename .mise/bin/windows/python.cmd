@echo off
rem The emsdk vfox plugin calls python before emsdk installs its bundled copy.
rem Resolve the project interpreter without relying on install-hook PATH order.
for /f "delims=" %%i in ('mise where python') do set "MISE_PROJECT_PYTHON=%%i\python.exe"
if not defined MISE_PROJECT_PYTHON (
  echo mise could not locate the project Python interpreter. 1>&2
  exit /b 1
)
"%MISE_PROJECT_PYTHON%" %*
