# Offline password manager

A CLI-only password manager, which stores your passwords in a local file, encrypted
with a master password. Requires OpenSSL >= 3 to be installed on the system,
for encryption.

Supports basic CRUD operations on the password list, such as creating a new
password entry, listing all entries or removing an entry. Passwords are
automatically copied to clipboard.

To build the executable, run:

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Run `./pass` afterwards to see the help text.
