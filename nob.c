#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "build"
#define TARGET    BUILD_DIR "/punktop"

static const char *cxxflags[] = {
    "-Wall",
    "-Wextra",
    "-Iinclude",
    "-Iextras",
    "-Isrc/imgui",
    "-Isrc/implot",
};

static const char *ldlibs[] = {
    "-Llib",
    "-lraylib",
    "-lX11",
    "-lXrandr",
    "-lXi",
    "-lXcursor",
    "-lXinerama",
    "-lGL",
    "-lm",
    "-ldl",
    "-lpthread",
};

static bool collect_sources_cb(Nob_Walk_Entry entry)
{
    if (entry.type != NOB_FILE_DIRECTORY) {
        size_t len = strlen(entry.path);
        if (len >= 4 && strcmp(entry.path + len - 4, ".cpp") == 0)
            nob_da_append((Nob_File_Paths *) entry.data, nob_temp_strdup(entry.path));
    }
    return true;
}

static bool build(void)
{
    Nob_File_Paths srcs = {0};
    Nob_File_Paths objs = {0};

    if (!nob_walk_dir("src", collect_sources_cb, .data = &srcs))
        return false;

    nob_mkdir_if_not_exists(BUILD_DIR);

    for (size_t i = 0; i < srcs.count; ++i) {
        const char *src = srcs.items[i];
        char *obj = nob_temp_sprintf("%s/%s", BUILD_DIR, src + 4);
        strcpy(strrchr(obj, '.'), ".o");

        if (!nob_mkdir_if_not_exists(nob_temp_dir_name(obj)))
            return false;

        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "clang++");
        nob_da_append_many(&cmd, cxxflags, NOB_ARRAY_LEN(cxxflags));
        nob_cmd_append(&cmd, "-c", src, "-o", obj);

        if (!nob_cmd_run(&cmd))
            return false;

        nob_da_append(&objs, obj);
    }

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "clang++");
    nob_da_append_many(&cmd, objs.items, objs.count);
    nob_da_append_many(&cmd, ldlibs, NOB_ARRAY_LEN(ldlibs));
    nob_cmd_append(&cmd, "-o", TARGET);

    return nob_cmd_run(&cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "clean") == 0) {
        nob_delete_file(TARGET);
        return 0;
    }

    if (!build())
        return 1;

    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "./" TARGET);
        return nob_cmd_run(&cmd) ? 0 : 1;
    }

    return 0;
}
