// Year      : 2020
// Subject   : SystemProgramming
// title     : ls –Rl programming
// StudentID : B511032
// Name      : SungJo Kim

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

char init_path[1024] = "";

int type_print(mode_t file_type)
{
    if      (S_ISREG (file_type)) printf("-");
    else if (S_ISDIR (file_type)) printf("d");
    else if (S_ISCHR (file_type)) printf("c");
    else if (S_ISBLK (file_type)) printf("b");
    else if (S_ISFIFO(file_type)) printf("f");
    else if (S_ISLNK (file_type)) printf("l");
    else if (S_ISSOCK(file_type)) printf("s");
    else                          return -1;

    return 0;
}

int auth_print(mode_t file_auth)
{
    int uid_flag = 0;
    int gid_flag = 0;
    int sti_flag = 0;

    if (S_ISUID & file_auth) uid_flag = 1;
    if (S_ISGID & file_auth) gid_flag = 1;
    if (S_ISVTX & file_auth) sti_flag = 1;

    if (S_IRUSR & file_auth) printf("r"); else printf("-");
    if (S_IWUSR & file_auth) printf("w"); else printf("-");
    if (S_IXUSR & file_auth) {
        if (uid_flag == 1)   printf("s"); else printf("x");
    } else {
        if (uid_flag == 1)   printf("S"); else printf("-");
    }

    if (S_IRGRP & file_auth) printf("r"); else printf("-");
    if (S_IWGRP & file_auth) printf("w"); else printf("-");
    if (S_IXGRP & file_auth) {
        if (gid_flag == 1)   printf("s"); else printf("x");
    } else {
        if (gid_flag == 1)   printf("S"); else printf("-");
    }

    if (S_IROTH & file_auth) printf("r"); else printf("-");
    if (S_IWOTH & file_auth) printf("w"); else printf("-");
    if (S_IXOTH & file_auth) {
        if (sti_flag == 1)   printf("t"); else printf("x");
    } else {
        if (sti_flag == 1)   printf("T"); else printf("-");
    }

    return 0;
}

int size_or_device_number_print(struct stat file)
{
    if (S_ISCHR(file.st_mode) || S_ISBLK(file.st_mode)) {
        printf(" %ld, %ld ", major(file.st_rdev), minor(file.st_rdev));
    }
    else printf(" %lld ", file.st_size);

    return 0;
}

int mtime_print(time_t file_time)
{
    char   mtime[32] = "";
    struct tm buf;

    if (localtime_r(&file_time, &buf) == NULL)
        return -1;
    strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M", &buf);
    printf("%s", mtime);

    return 0;
}

int filename_print(char *file_name, mode_t file_type)
{
    char buf_link_name[1024] = "";

    if (S_ISLNK(file_type)) {
        if (readlink(file_name, buf_link_name, sizeof(buf_link_name)) < 0)
            return  -1;
        printf(" %s -> %s\n", file_name, buf_link_name);
    }
    else
        printf(" %s\n", file_name);

    return 0;
}

int lstat_function(char *file_name)
{
    struct stat   buf;
    struct passwd *u_name;
    struct group  *g_name;
    struct tm     file_mtime;

    // save file_name info in buf
    if (lstat(file_name, &buf) < 0) {
        printf("Error : lstat()"); return -1;
    }

    // confirm of [file_type]
    if (type_print(buf.st_mode) < 0) {
        printf("Error : type_print()"); return -1;
    }

    // confirm of [file access permissions]
    if (auth_print(buf.st_mode) < 0) {
        printf("Error : auth_print()"); return -1;
    }

    // confirm of [user name & group name]
    u_name = getpwuid(buf.st_uid);
    g_name = getgrgid(buf.st_gid);

    // number of links, user name, group name, file size
    printf(" %ld %s %s",
            buf.st_nlink, u_name->pw_name, g_name->gr_name);

    // confirm of [file size] or [major & minor device number]
    if (size_or_device_number_print(buf) < 0) {
        printf("Error : size_or_device_number_print()"); return -1;
    }

    // last modification time of file
    if (mtime_print(buf.st_mtime) < 0) {
        printf("Error : mtime_print()"); return -1;
    }

    // confirm of [real file name]
    if (filename_print(file_name, buf.st_mode) < 0) {
        printf("Error : filename_print()"); return -1;
    }

    return 0;
}

void relative_path_print(char *file_name)
{
    int i = 0;

    if (init_path != file_name) {
        while (init_path[i] == file_name[i]) { i++; }

        printf(".");
        while (file_name[i]) {
            printf("%c", file_name[i]);
            i++;
        }   printf(":");
    } else  printf(".:");
}

int total_print(char *file_name)
{
    char     buffer[1024];
    int      count, i;
    int      total = 0;
    struct   dirent **file;
    struct   stat   buf;

    if ((count = scandir(file_name, &file, NULL, alphasort)) == -1) {
        printf("Error : scandir()\n"); return -1;
    }

    for (i = 0; i < count; i++) {
        if (file[i]->d_name[0] == '.') continue;
        if (lstat(file[i]->d_name, &buf) < 0) {
            printf("Error : lstat()"); return -1;
        }
        total += buf.st_blocks;
    } printf("total %d\n", total / 2);

    return 0;
}

int type_is_dir(char *file_name)
{
    struct stat buf;

    if (lstat(file_name, &buf) < 0) {
        printf("Error : lstat() : %s", file_name); return -1;
    }

    if (S_ISDIR(buf.st_mode)) return 1;
    else                      return 0;
}

int recursive(char *dir_path)
{
    char   buffer[1024];
    int    count, i, num;
    struct dirent **file;

    // change current directory to dir_path
    if (chdir(dir_path) < 0) {
        printf("Error : chdir()\n"); return -1;
    }

    // scan all files arranged in alphabetical in dir_path
    if ((count = scandir(dir_path, &file, NULL, alphasort)) == -1) {
        printf("ls: cannot open directory ");
        relative_path_print(dir_path);
        printf(" Permission denied\n");
        return -1;
    }

    // confirm of [relative path for directory file]
    relative_path_print(dir_path);
    printf("\n");

    // confirm of [total for directory file]
    if (total_print(dir_path) < 0) {
        printf("Error : total_print()\n"); return -1;
    }

    // confirm of [file list]
    for (i = 0; i < count; i++) {
        if (file[i]->d_name[0] == '.') continue;
        if (lstat_function(file[i]->d_name) < 0) {
            printf(" in lstat_function() named %s\n", file[i]->d_name); return -1;
        }
    } printf ("\n");

    // confirm of [file list] again for finding directory file
    for (i = 0; i < count; i++) {
        if (file[i]->d_name[0] == '.') continue;

        if (chdir(dir_path) < 0) {
            printf("Error : chdir()\n"); return -1;
        }
        if ((num = type_is_dir(file[i]->d_name)) < 0) {
            printf(" in type_is_dir() named %s\n", file[i]->d_name); return -1;
        } else if (num > 0) {
            if (realpath(file[i]->d_name, buffer) == NULL) {
                printf("Error : realpath()\n"); return -1;
            }
            recursive(buffer);
        }
    }

    // memory free
    for (i = 0; i < count; i++)
        free(file[i]);
    free(file);

    return 0;
}

int main(int argc, char *argv[])
{
    // Usage
    if (argc > 1) {
        printf("Usage: %s\n", argv[0]); return -1;
    }

    // pwd is initial path
    if (getcwd(init_path, sizeof(init_path)) == NULL) {
        printf("Error : getcwd()\n"); return -1;
    }

    // start recursive for directory
    if (recursive(init_path) < 0) {
        return -1;
    }

    return 0;
}
