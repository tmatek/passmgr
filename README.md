# Offline password manager

A CLI-only password manager, which stores your passwords in a local file, encrypted
with a master password. Requires OpenSSL >= 1.1.1 to be installed on the system,
for encryption.

Supports basic CRUD operations on the password list, such as creating a new
password entry, listing all entries or removing an entry. Passwords are
automatically copied to clipboard.

To build the executable, run:

```
mkdir build
cd build
cmake ..
make
```

Run `./pass` afterwards to see the help text.
