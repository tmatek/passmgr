// I picked these limits as it makes memory management easier;
// I do not anticipate more than 1000 passwords per database
// or passwords + identifiers so long, that they would exceed
// the 200 character line limit.

// maximum 200 characters per line
typedef char Line[200];

// maximum 1000 lines per database
typedef Line Lines[1000];

// maximum 50 character passwords
#define PASSWD_MAX_LENGTH 50

// maximum characters in a file-system path
#define FS_MAX_PATH_LENGTH 1024
