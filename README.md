# CEnv

### Simple C library used to load environment variables from .env files

Main functions:
``` c 
load_env_vars(const char* path);
unload_env_vars();
```

```path``` **needs** to be an absolute path, get it with
```c 
char* realpath(const char* relPath, char* absPath);
```
on **nix systems*

or with
```c
char* __fullpath(char* absPath, const char* relPath, size_t maxLen);
```
on *Windows*

Make sure to call ```unload_env_paths();``` at the end to free up malloc-ed memory!
