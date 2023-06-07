cmake_minimum_required(VERSION 3.12)
project(passmgr)

IF (WIN32)
  add_executable(pass main.c ipc-win.c)
ELSE ()
  add_executable(pass newmain.c inout.c database.c password.c ipc-unix.c)
ENDIF()
