{
  "targets": [{
    "target_name": "sdjournal",
    "sources": [
      "src/addon.c",
      "src/writer.c",
      "src/reader.c"
    ],
    "libraries": ["-lsystemd"],
    "cflags": ["-Wall", "-Wextra", "-std=c11"],
    "include_dirs": ["src"]
  }]
}
