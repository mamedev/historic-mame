/* junko fakeo thing so we don't have to change the base source,
   and yet have mame still work with win32! - Chris Kirmse */

typedef char DIR;
struct dirent
{
   int blah;
   char d_name[20];
};

DIR * opendir(const char *basename);
struct dirent * readdir(DIR *dirp);
void closedir(DIR *dirp);

