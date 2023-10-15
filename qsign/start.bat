@echo off
set p="%cd%\jre\bin"
set path=%p%;%path%
:st
bin/unidbg-fetch-qsign --basePath=txlib/8.9.73