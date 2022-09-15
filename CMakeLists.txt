cmake_minimum_required(VERSION 2.8)
project(passmgr)

IF (WIN32)
  add_executable(pass main.c ipc-win.c)
ELSE ()
  add_executable(pass newmain.c cmdline.c)
ENDIF()
