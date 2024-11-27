// return 1 if uri is already on favorite list, 0 otherwise
int on_favorites(char *uri);

// return 1 if uri is on blacklist, 0 otherwise
int on_blacklist (char *uri);

// return 1 if uri has bad format, 0 otherwise
int bad_format (char *uri);

// init blacklist data structure, now inside util
void init_blacklist (char *fname);

